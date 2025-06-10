#include <algorithm>
#include <numeric>

#include "llvm/IR/AttributeMask.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "memoir/ir/Builder.hpp"
#include "memoir/lowering/Implementation.hpp"
#include "memoir/support/SortedVector.hpp"
#include "memoir/support/WorkList.hpp"
#include "memoir/transforms/utilities/MutateType.hpp"
#include "memoir/transforms/utilities/ReifyTempArgs.hpp"
#include "memoir/utility/Metadata.hpp"

#include "folio/transforms/CoalesceUses.hpp"
#include "folio/transforms/ProxyInsertion.hpp"
#include "folio/transforms/Utilities.hpp"
#include "folio/transforms/WeakenUses.hpp"

using namespace llvm::memoir;

namespace folio {

// ================
static llvm::cl::opt<bool> disable_translation_elimination(
    "disable-translation-elimination",
    llvm::cl::desc("Disable redundant translation elimination"),
    llvm::cl::init(false));

static llvm::cl::opt<bool> disable_use_weakening(
    "disable-use-weakening",
    llvm::cl::desc("Disable weakening uses"),
    llvm::cl::init(true));

static llvm::cl::opt<bool> disable_proxy_propagation(
    "disable-proxy-propagation",
    llvm::cl::desc("Disable proxy propagation"),
    llvm::cl::init(false));

static llvm::cl::opt<bool> disable_total_proxy(
    "disable-total-proxy",
    llvm::cl::desc("Disable total proxy optimization"),
    llvm::cl::init(false));

static llvm::cl::opt<bool> disable_proxy_sharing(
    "disable-proxy-sharing",
    llvm::cl::desc("Disable proxy sharing optimization"),
    llvm::cl::init(false));

static llvm::cl::opt<std::string> proxy_set_impl(
    "proxy-set-impl",
    llvm::cl::desc("Set the implementation for proxied sets"),
    llvm::cl::init("bitset"));

static llvm::cl::opt<std::string> proxy_nested_set_impl(
    "proxy-nested-set-impl",
    llvm::cl::desc("Set the implementation for proxied sets that are nested"),
    llvm::cl::init(proxy_set_impl));

static llvm::cl::opt<std::string> proxy_map_impl(
    "proxy-map-impl",
    llvm::cl::desc("Set the implementation for proxied map"),
    llvm::cl::init("bitmap"));

void ProxyInsertion::gather_assoc_objects(Vector<ObjectInfo> &allocations,
                                          AllocInst &alloc,
                                          Type &type,
                                          Vector<unsigned> offsets) {

  if (auto *tuple_type = dyn_cast<TupleType>(&type)) {
    for (unsigned field = 0; field < tuple_type->getNumFields(); ++field) {

      auto new_offsets = offsets;
      new_offsets.push_back(field);

      this->gather_assoc_objects(allocations,
                                 alloc,
                                 tuple_type->getFieldType(field),
                                 new_offsets);
    }

  } else if (auto *collection_type = dyn_cast<CollectionType>(&type)) {
    auto &elem_type = collection_type->getElementType();

    // If this is an assoc, add the object information.
    if (isa<AssocType>(collection_type)) {
      allocations.push_back(ObjectInfo(alloc, offsets));
    }

    // Recurse on the element.
    auto new_offsets = offsets;
    new_offsets.push_back(-1);

    this->gather_assoc_objects(allocations, alloc, elem_type, new_offsets);
  }

  return;
}

static AllocInst *_find_base_object(llvm::Value &V,
                                    Vector<unsigned> &offsets,
                                    Set<llvm::Value *> &visited) {
  if (visited.count(&V) > 0) {
    return nullptr;
  } else {
    visited.insert(&V);
  }

  if (auto *arg = dyn_cast<llvm::Argument>(&V)) {
    auto &func =
        MEMOIR_SANITIZE(arg->getParent(), "Argument has no parent function");
    if (auto *fold = FoldInst::get_single_fold(func)) {
      if (arg == fold->getElementArgument()) {
        auto it = offsets.begin();
        for (auto *index : fold->indices()) {
          unsigned offset = -1;
          if (auto *index_const = dyn_cast<llvm::ConstantInt>(index)) {
            offset = index_const->getZExtValue();
          }
          it = offsets.insert(it, offset);
        }
        offsets.insert(it, -1);

        return _find_base_object(fold->getObject(), offsets, visited);
      } else if (auto *operand = fold->getOperandForArgument(*arg)) {
        return _find_base_object(*operand->get(), offsets, visited);
      }
    }

  } else if (auto *phi = dyn_cast<llvm::PHINode>(&V)) {
    for (auto &incoming : phi->incoming_values()) {
      auto *base = _find_base_object(*incoming.get(), offsets, visited);
      if (base) {
        return base;
      }
    }
  } else if (auto *alloc = into<AllocInst>(&V)) {
    return alloc;

  } else if (auto *update = into<UpdateInst>(&V)) {
    return _find_base_object(update->getObject(), offsets, visited);

  } else if (auto *ret_phi = into<RetPHIInst>(&V)) {
    return _find_base_object(ret_phi->getInput(), offsets, visited);

  } else if (auto *call = dyn_cast<llvm::CallBase>(&V)) {
    // TODO
    warnln("Base object returned from call!");
  }

  return nullptr;
}

ObjectInfo *ProxyInsertion::find_base_object(llvm::Value &V,
                                             AccessInst &access) {

  infoln("FINDING BASE OF ", access);

  auto *type = type_of(V);

  Vector<unsigned> offsets = {};
  for (auto *index : access.indices()) {
    if (auto *tuple_type = dyn_cast<TupleType>(type)) {
      auto index_const = dyn_cast<llvm::ConstantInt>(index);
      auto field = index_const->getZExtValue();
      type = &tuple_type->getFieldType(field);

      offsets.push_back(field);

    } else if (auto *collection_type = dyn_cast<CollectionType>(type)) {
      type = &collection_type->getElementType();

      offsets.push_back(-1);
    }
  }
  if (isa<FoldInst>(&access)) {
    offsets.push_back(-1);
  }

  Set<llvm::Value *> visited = {};

  auto *alloc = _find_base_object(V, offsets, visited);

  if (not alloc) {
    return nullptr;
  }

  this->propagators.emplace_back(*alloc, offsets);
  auto &info = this->propagators.back();

  infoln("FOUND PROPAGATOR ", info);

  return &info;
}

void ProxyInsertion::gather_propagators(
    Map<llvm::Function *, Set<llvm::Value *>> encoded,
    Map<llvm::Function *, Set<llvm::Use *>> to_encode) {

  for (const auto &[func, values] : encoded) {
    for (auto *val : values) {
      for (auto &use : val->uses()) {
        auto *user = dyn_cast<llvm::Instruction>(use.getUser());
        if (not user) {
          continue;
        }

        infoln("GATHER ", *user);

        if (auto *write = into<WriteInst>(user)) {
          if (&use == &write->getValueWrittenAsUse()) {
            this->find_base_object(write->getObject(), *write);
          }

        } else if (auto *insert = into<InsertInst>(user)) {
          if (auto value_keyword = insert->get_keyword<ValueKeyword>()) {
            if (&use == &value_keyword->getValueAsUse()) {
              this->find_base_object(insert->getObject(), *insert);
            }
          }
        }
      }
    }
  }

  for (const auto &[func, uses] : to_encode) {
    for (auto *use : uses) {
      auto *user = dyn_cast<llvm::Instruction>(use->getUser());
      if (not user) {
        continue;
      }

      auto *used = use->get();

      infoln("GATHER ", *user);

      if (auto *arg = dyn_cast<llvm::Argument>(used)) {
        auto &parent = MEMOIR_SANITIZE(arg->getParent(),
                                       "Argument has no parent function.");

        if (auto *fold = FoldInst::get_single_fold(parent)) {
          if (arg == fold->getElementArgument()) {
            this->find_base_object(fold->getObject(), *fold);
          }
        }

      } else if (auto *read = into<ReadInst>(used)) {
        this->find_base_object(read->getObject(), *read);
      }
    }
  }

  // Deduplicate propagators.
  for (auto it = this->propagators.begin(); it != this->propagators.end();) {
    auto &info = *it;

    // Search for an equivalent propagator.
    auto found = std::find_if(this->propagators.begin(),
                              it,
                              [&](const ObjectInfo &other) {
                                return info.allocation == other.allocation
                                       and std::equal(info.offsets.begin(),
                                                      info.offsets.end(),
                                                      other.offsets.begin(),
                                                      other.offsets.end());
                              });

    if (found != it) {
      // If we found an equivalent propagator, delete this one.
      it = this->propagators.erase(it);
    } else {
      // Analyze this object, since it is unique.
      info.analyze();
      // Commit this object.
      ++it;
    }
  }

  return;
}

void ProxyInsertion::analyze() {
  auto &M = this->M;

  // Gather all possible allocations.
  for (auto &F : M) {
    for (auto &BB : F) {
      for (auto &I : BB) {
        if (auto *alloc = into<AllocInst>(&I)) {
          // Gather all of the Assoc allocations.
          this->gather_assoc_objects(this->objects,
                                     *alloc,
                                     alloc->getType(),
                                     {});
        }
      }
    }
  }

  println();
  println("FOUND OBJECTS ", this->objects.size());
  for (auto &info : this->objects) {
    println("  ", info);
  }
  println();

  // Analyze each of the objects.
  for (auto &info : this->objects) {
    info.analyze();

    forward_analysis(info.encoded);
  }

  // With the set of values that need to be encoded/decoded, we will find
  // collections that can be used to propagate proxied values.
  if (not disable_proxy_propagation) {

    // From the use information, find any collection elements that _could_
    // propagate the proxy.
    Map<llvm::Function *, Set<llvm::Value *>> encoded = {};
    Map<llvm::Function *, Set<llvm::Use *>> to_encode = {};
    for (auto &info : this->objects) {
      for (const auto &[func, locals] : info.encoded) {
        encoded[func].insert(locals.begin(), locals.end());
      }
      for (const auto &uses : { info.to_encode, info.to_addkey }) {
        for (const auto &[func, locals] : uses) {
          to_encode[func].insert(locals.begin(), locals.end());
        }
      }
    }

    // Gather the propagators from the encoded values.
    this->gather_propagators(encoded, to_encode);

    println();
    println("FOUND PROPAGATORS ", this->propagators.size());
    for (auto &info : this->propagators) {
      println("  ", info);
    }
    println();
  }

  // Use a heuristic to share proxies between collections.
  Set<const ObjectInfo *> used = {};
  for (auto it = this->objects.begin(); it != this->objects.end(); ++it) {
    auto &info = *it;

    if (used.count(&info)) {
      continue;
    }

    auto *alloc = info.allocation;
    auto *bb = alloc->getParent();
    auto *func = bb->getParent();

    auto &type = MEMOIR_SANITIZE(dyn_cast<AssocType>(&info.get_type()),
                                 "Non-assoc type, unhandled.");

    this->candidates.emplace_back();
    auto &candidate = this->candidates.back();
    candidate.push_back(&info);

    // Find all other allocations in the same function as this one.
    if (not disable_proxy_sharing) {
      for (auto it2 = std::next(it); it2 != this->objects.end(); ++it2) {
        auto &other = *it2;

        if (used.count(&other)) {
          continue;
        }

        // Check that the key types match.
        auto *other_alloc = other.allocation;
        auto &other_type =
            MEMOIR_SANITIZE(dyn_cast<AssocType>(&other.get_type()),
                            "Non-assoc type, unhandled.");

        if (&type.getKeyType() != &other_type.getKeyType()) {
          continue;
        }

        // Check that they share a parent function.
        // NOTE: this is overly conservative
        auto *other_func = other.allocation->getFunction();
        if (func != other_func) {
          continue;
        }

        candidate.push_back(&other);
      }

      // Find all propagators in the same function as this one.
      for (auto &other : this->propagators) {
        if (used.count(&other) > 0) {
          continue;
        }

        auto *other_alloc = other.allocation;
        auto &other_type = other.get_type();

        if (&type.getKeyType() != &other_type) {
          continue;
        }

        // Check that they share a parent basic block.
        auto *other_func = other_alloc->getFunction();
        if (func != other_func) {
          continue;
        }

        candidate.push_back(&other);
      }
    }

    // Compute the benefit of each object in the candidate.
    uint32_t candidate_benefit = 0;
    Set<const ObjectInfo *> has_benefit = {};
    for (const auto *info : candidate) {
      for (const auto *other : candidate) {
        if (info == other) {
          continue;
        }

        auto benefit = info->compute_heuristic(*other);

        if (benefit > 0) {
          has_benefit.insert(info);
          has_benefit.insert(other);
          candidate_benefit += benefit;
        }
      }
    }

    // Remove objects that provide no benefits.
    std::erase_if(candidate, [&](ObjectInfo *info) {
      return has_benefit.count(info) == 0;
    });

    // If there is no benefit, roll back the candidate.
    if (candidate.size() == 0) {
      candidates.pop_back();
    } else {

      // Mark the objects in the candidate as being used.
      used.insert(candidate.begin(), candidate.end());

      println("CANDIDATE:");
      println("  BENEFIT=", candidate_benefit);
      for (const auto *info : candidate) {
        println("  ", *info);
      }
    }
  }
}

static llvm::FunctionCallee create_addkey_function(llvm::Module &M,
                                                   Type &key_type,
                                                   bool build_encoder,
                                                   Type *encoder_type,
                                                   bool build_decoder,
                                                   Type *decoder_type) {

  auto &context = M.getContext();
  auto &data_layout = M.getDataLayout();

  auto &size_type = Type::get_size_type(data_layout);

  auto *llvm_size_type = size_type.get_llvm_type(context);
  auto *llvm_ptr_type = llvm::PointerType::get(context, 0);
  auto *llvm_key_type = key_type.get_llvm_type(context);

  // Create the addkey functions for this proxy.
  Vector<llvm::Type *> addkey_params = { llvm_key_type };
  if (build_encoder) {
    addkey_params.push_back(llvm_ptr_type);
  }
  if (build_decoder) {
    addkey_params.push_back(llvm_ptr_type);
  }
  auto *addkey_type =
      llvm::FunctionType::get(llvm_size_type, addkey_params, false);
  auto &addkey_function = MEMOIR_SANITIZE(
      llvm::Function::Create(addkey_type,
                             llvm::GlobalValue::LinkageTypes::InternalLinkage,
                             "proxy_addkey_",
                             M),
      "Failed to create LLVM function");

  auto arg_idx = 0;
  auto *key = addkey_function.getArg(arg_idx++);

  llvm::Argument *encoder = nullptr;
  if (build_encoder) {
    encoder = addkey_function.getArg(arg_idx++);
  }

  llvm::Argument *decoder = nullptr;
  if (build_decoder) {
    decoder = addkey_function.getArg(arg_idx++);
  }

  auto *ret_bb = llvm::BasicBlock::Create(context, "", &addkey_function);

  MemOIRBuilder builder(ret_bb);

  if (build_encoder) {
    builder.CreateAssertTypeInst(encoder, *encoder_type);
  }

  if (build_decoder) {
    builder.CreateAssertTypeInst(decoder, *decoder_type);
  }

  auto *has_key = &builder.CreateHasInst(encoder, key)->getCallInst();
  auto *phi = builder.CreatePHI(llvm_size_type, 2);
  llvm::PHINode *encoder_phi = nullptr;
  if (build_encoder) {
    encoder_phi = builder.CreatePHI(llvm_ptr_type, 2);
  }

  llvm::PHINode *decoder_phi = nullptr;
  if (build_decoder) {
    decoder_phi = builder.CreatePHI(llvm_ptr_type, 2);
  }

  auto *ret = builder.CreateRet(phi);

  llvm::Instruction *then_terminator, *else_terminator;
  llvm::SplitBlockAndInsertIfThenElse(has_key,
                                      phi,
                                      &then_terminator,
                                      &else_terminator);

  // if (has(encoder, key)) {
  //   z2 = read(encoder, key)
  // }
  auto *then_bb = then_terminator->getParent();
  builder.SetInsertPoint(then_terminator);
  auto *read_index =
      &builder.CreateReadInst(size_type, encoder, { key })->getCallInst();

  // else {
  //   z1 = size(encoder)
  //   insert(size(encoder), encoder, key)
  //   insert(key, decoder, end)
  // }
  auto *else_bb = else_terminator->getParent();
  builder.SetInsertPoint(else_terminator);
  llvm::Value *new_index = nullptr;
  llvm::Value *new_encoder = nullptr;
  if (encoder) {
    new_index = &builder.CreateSizeInst(encoder)->getCallInst();
    auto *insert = builder.CreateInsertInst(encoder, { key });
    new_encoder = &builder
                       .CreateWriteInst(size_type,
                                        new_index,
                                        &insert->getCallInst(),
                                        { key })
                       ->getCallInst();
  }

  llvm::Value *new_decoder = nullptr;
  if (decoder) {
    if (not new_index) {
      new_index = &builder.CreateSizeInst(decoder)->getCallInst();
    }
    auto *end = &builder.CreateEndInst()->getCallInst();
    auto *insert = builder.CreateInsertInst(decoder, { end });
    new_decoder = &builder
                       .CreateWriteInst(key_type,
                                        key,
                                        &insert->getCallInst(),
                                        { new_index })
                       ->getCallInst();
  }

  // Update the PHIs
  phi->addIncoming(read_index, then_bb);
  phi->addIncoming(new_index, else_bb);

  if (encoder_phi) {
    encoder_phi->addIncoming(encoder, then_bb);
    encoder_phi->addIncoming(new_encoder, else_bb);
    auto encoder_live_out = Metadata::get_or_add<LiveOutMetadata>(*encoder_phi);
    encoder_live_out.setArgNo(encoder->getArgNo());
  }

  if (decoder_phi) {
    decoder_phi->addIncoming(decoder, then_bb);
    decoder_phi->addIncoming(new_decoder, else_bb);
    auto decoder_live_out = Metadata::get_or_add<LiveOutMetadata>(*decoder_phi);
    decoder_live_out.setArgNo(decoder->getArgNo());
  }

  return llvm::FunctionCallee(&addkey_function);
}

static void collect_callers(llvm::Function &to,
                            Set<llvm::Function *> &functions) {

  if (functions.count(&to) > 0) {
    return;
  }

  functions.insert(&to);

  // Collect all direct callers of this function.
  for (auto &use : to.uses()) {
    auto *call = dyn_cast<llvm::CallBase>(use.getUser());
    if (not call) {
      continue;
    }
    auto *memoir = into<MemOIRInst>(call);
    auto *fold = dyn_cast_or_null<FoldInst>(memoir);
    if (memoir and not fold) {
      continue;
    }

    // Ensure that this is the function being called.
    auto *callee = fold ? &fold->getBody() : call->getCalledFunction();
    if (callee != &to) {
      continue;
    }

    // Insert the parent function.
    collect_callers(*call->getFunction(), functions);
  }
}

static void collect_callees(llvm::Function &from,
                            Set<llvm::Function *> &functions) {
  if (functions.count(&from) > 0) {
    return;
  }

  functions.insert(&from);

  // Collect all direct callees from this function.
  for (auto &BB : from) {
    for (auto &I : BB) {
      auto *call = dyn_cast<llvm::CallBase>(&I);
      if (not call) {
        continue;
      }
      auto *memoir = into<MemOIRInst>(call);
      auto *fold = dyn_cast_or_null<FoldInst>(memoir);
      if (memoir and not fold) {
        continue;
      }

      auto *callee = fold ? &fold->getBody() : call->getCalledFunction();

      // Insert the called function.
      if (callee) {
        collect_callees(*callee, functions);
      }
    }
  }
}

static void add_tempargs(
    Map<llvm::Function *, llvm::Instruction *> &local_patches,
    llvm::ArrayRef<Set<llvm::Use *>> uses_to_patch,
    llvm::Instruction &patch_with,
    Type &patch_type,
    const llvm::Twine &name) {

  MemOIRBuilder builder(&patch_with);
  auto &context = builder.getContext();
  auto &module = builder.getModule();

  // Unpack the patch.
  auto *patch_func = patch_with.getFunction();
  auto *llvm_patch_type = patch_type.get_llvm_type(context);

  // Track the local patch for each function.
  local_patches[patch_func] = &patch_with;

  // Find the set of functions that need the patch.
  Set<llvm::Function *> functions = { patch_func };
  for (auto &uses : uses_to_patch) {
    for (auto *use : uses) {
      if (auto *inst = dyn_cast<llvm::Instruction>(use->getUser())) {
        functions.insert(inst->getFunction());
      }
    }
  }

  // Close the set of functions.
  Set<llvm::Function *> forward = {};
  Set<llvm::Function *> backward = {};
  for (auto *func : functions) {
    collect_callers(*func, forward);
    collect_callees(*func, backward);
  }

  for (auto *func : forward) {
    if (backward.count(func) > 0) {
      functions.insert(func);
    }
  }

  // Determine the set of functions that we need to pass the proxy to.
  Map<llvm::CallBase *, llvm::GlobalVariable *> calls_to_patch = {};
  Map<FoldInst *, llvm::GlobalVariable *> folds_to_patch = {};
  for (auto *func : functions) {

    if (func == patch_func) {
      continue;
    }

    // Create the global variable.
    auto *global = new llvm::GlobalVariable(
        module,
        llvm_patch_type,
        /* isConstant? */ false,
        llvm::GlobalValue::LinkageTypes::InternalLinkage,
        llvm::Constant::getNullValue(llvm_patch_type),
        "temparg");

    // Create the load in the entry of the fold body.
    builder.SetInsertPoint(func->getEntryBlock().getFirstNonPHI());

    auto *load = builder.CreateLoad(llvm_patch_type, global);
    Metadata::get_or_add<TempArgumentMetadata>(*load);

    load->setName(name);

    // Annotate the loaded value with type information.
    builder.CreateAssertTypeInst(load, patch_type);

    local_patches[func] = load;

    for (auto &use : func->uses()) {
      auto *call = dyn_cast<llvm::CallBase>(use.getUser());
      if (not call) {
        continue;
      }

      if (auto *fold = into<FoldInst>(call)) {
        if (&fold->getBody() == func) {
          folds_to_patch[fold] = global;
        }
      } else if (not into<MemOIRInst>(call)) {
        if (call->getCalledFunction() == func) {
          calls_to_patch[call] = global;
        }
      }
    }
  }

  // Patch each of the folds by storing to the global before the operation.
  for (const auto &[fold, global] : folds_to_patch) {
    // Unpack.
    auto &inst = fold->getCallInst();
    auto *func = inst.getFunction();

    // Create the store ahead of the fold.
    builder.SetInsertPoint(&inst);

    auto *local = local_patches[func];

    if (not local) {
      warnln("No local patch for caller ", func->getName());
      continue;
    }

    auto *store = builder.CreateStore(local, global);
    Metadata::get_or_add<TempArgumentMetadata>(*store);
  }

  // Patch each of the calls by storing to the global before the operation.
  for (const auto &[call, global] : calls_to_patch) {
    // Unpack.
    auto *func = call->getFunction();

    // Create the store ahead of the call.
    builder.SetInsertPoint(call);

    auto *local = local_patches[func];

    if (not local) {
      warnln("No local patch for caller ", func->getName());
      continue;
    }

    auto *store = builder.CreateStore(local, global);
    Metadata::get_or_add<TempArgumentMetadata>(*store);
  }

  return;
}

/**
 * Following a function clone--where the old function will be deleted--update
 * any candidate information to point to the cloned function.
 */
static void update_candidates(std::forward_iterator auto candidates_begin,
                              std::forward_iterator auto candidates_end,
                              llvm::Function &old_func,
                              llvm::Function &new_func,
                              llvm::ValueToValueMapTy &vmap) {
  for (auto it = candidates_begin; it != candidates_end; ++it) {
    auto &candidate = *it;
    for (auto *info : candidate) {

      info->update(old_func, new_func, vmap, /* delete old? */ true);
    }
  }
}

static bool used_value_will_be_decoded(llvm::Use &use,
                                       const Set<llvm::Use *> &to_decode,
                                       Set<llvm::Use *> &visited) {

  debugln("DECODED? ", *use.get());

  if (to_decode.count(&use) > 0) {
    debugln("  YES");
    return true;
  }

  if (visited.count(&use) > 0) {
    debugln("  YES");
    return true;
  } else {
    visited.insert(&use);
  }

  auto *value = use.get();
  if (auto *phi = dyn_cast<llvm::PHINode>(value)) {
    debugln("  RECURSE");
    bool all_decoded = true;
    for (auto &incoming : phi->incoming_values()) {
      all_decoded &= used_value_will_be_decoded(incoming, to_decode, visited);
    }
    return all_decoded;
  } else if (auto *select = dyn_cast<llvm::SelectInst>(value)) {
    debugln("  RECURSE");
    return used_value_will_be_decoded(select->getOperandUse(1),
                                      to_decode,
                                      visited)
           and used_value_will_be_decoded(select->getOperandUse(2),
                                          to_decode,
                                          visited);
  } else if (auto *arg = dyn_cast<llvm::Argument>(value)) {
    auto &func =
        MEMOIR_SANITIZE(arg->getParent(), "Argument has no parent function!");
    if (auto *fold = FoldInst::get_single_fold(func)) {
      if (auto *operand_use = fold->getOperandForArgument(*arg)) {
        debugln("  RECURSE");
        return used_value_will_be_decoded(*operand_use, to_decode, visited);
      }
    }
  }

  debugln("  NO");
  return false;
}

static bool used_value_will_be_decoded(llvm::Use &use,
                                       const Set<llvm::Use *> &to_decode) {
  Set<llvm::Use *> visited = {};
  return used_value_will_be_decoded(use, to_decode, visited);
}

static void update_use(llvm::Use &use, llvm::Value &value) {

  use.set(&value);

  if (auto *call = dyn_cast<llvm::CallBase>(use.getUser())) {
    auto operand_no = use.getOperandNo();
    if (operand_no < call->arg_size()) {
      call->removeParamAttr(operand_no, llvm::Attribute::AttrKind::NonNull);
    }
  }

  return;
}

static llvm::Value &encode_use(
    MemOIRBuilder &builder,
    llvm::Use &use,
    std::function<llvm::Value &(llvm::Value &)> get_encoder,
    std::function<llvm::Value &(MemOIRBuilder &, llvm::Value &)> encode_value) {

  auto *user = cast<llvm::Instruction>(use.getUser());
  auto *used = use.get();

  auto &module = MEMOIR_SANITIZE(user->getModule(), "User has no module.");
  const auto &data_layout = module.getDataLayout();
  auto &context = user->getContext();

  auto &size_type = Type::get_size_type(data_layout);
  auto &llvm_size_type = *size_type.get_llvm_type(context);

  auto &encoder = get_encoder(*user);

  // Handle has operations separately.
  if (auto *has = into<HasInst>(user)) {
    if (is_last_index(&use, has->index_operands_end())) {
      // Construct an if-else block.
      // if (has(encoder, key))
      //   i = read(encoder, key)
      //   h = has(collection, i)
      // h' = PHI(h, false)

      auto *cond = &builder.CreateHasInst(&encoder, used)->getCallInst();
      auto *phi = builder.CreatePHI(cond->getType(), 2);

      // i' = PHI(i, undef)
      auto *index_phi = builder.CreatePHI(&llvm_size_type, 2);

      auto *then_terminator =
          llvm::SplitBlockAndInsertIfThen(cond,
                                          phi,
                                          /* unreachable? */ false);

      user->moveBefore(then_terminator);

      builder.SetInsertPoint(user);

      auto &encoded = encode_value(builder, *used);

      update_use(use, encoded);

      user->replaceAllUsesWith(phi);

      auto *then_bb = then_terminator->getParent();
      auto *else_bb = cond->getParent();

      phi->addIncoming(user, then_bb);
      phi->addIncoming(llvm::ConstantInt::getFalse(context), else_bb);

      index_phi->addIncoming(&encoded, then_bb);
      index_phi->addIncoming(llvm::UndefValue::get(&llvm_size_type), else_bb);

      return *index_phi;
    }
  }

  // In the common case, update the use with the encoded value.
  auto &encoded = encode_value(builder, *used);

  update_use(use, encoded);

  return encoded;
}

static void inject(
    llvm::LLVMContext &context,
    Vector<CoalescedUses> &decoded,
    Vector<CoalescedUses> &encoded,
    Vector<CoalescedUses> &added,
    std::function<llvm::Value &(llvm::Value &)> get_encoder,
    std::function<llvm::Value &(MemOIRBuilder &, llvm::Value &)> decode_value,
    std::function<llvm::Value &(MemOIRBuilder &, llvm::Value &)> encode_value,
    std::function<llvm::Value &(MemOIRBuilder &, llvm::Value &)> add_value) {

  for (auto &uses : encoded) {

    // Unpack the use.
    auto *use = uses.front();
    auto *used = use->get();
    auto *user = dyn_cast<llvm::Instruction>(use->getUser());

    // Compute the insertion point.
    llvm::Instruction *program_point = nullptr;
    if (user) {
      if (auto *phi = dyn_cast<llvm::PHINode>(user)) {
        MEMOIR_UNREACHABLE("En/decoding uses by PHI is unhandled.");
      } else {
        program_point = user;
      }

    } else {
      MEMOIR_UNREACHABLE("Failed to find a point to encode the value!");
    }

    // Create the builder.
    MemOIRBuilder builder(program_point);

    auto &encoded = encode_use(builder, *use, get_encoder, encode_value);

    // Update the coalesced uses.
    for (auto *other_use : uses) {
      if (other_use == use) {
        continue;
      }

      uses.value(encoded);

      update_use(*other_use, encoded);
    }
  }

  for (auto &uses : added) {

    // Unpack the use.
    auto *use = uses.front();
    auto *used = use->get();
    auto *user = dyn_cast<llvm::Instruction>(use->getUser());

    // Compute the insertion point.
    llvm::Instruction *program_point = nullptr;
    if (user) {
      if (auto *phi = dyn_cast<llvm::PHINode>(user)) {
        MEMOIR_UNREACHABLE("En/decoding uses by PHI is unhandled.");
      } else {
        program_point = user;
      }

    } else {
      MEMOIR_UNREACHABLE("Failed to find a point to encode the value!");
    }

    MemOIRBuilder builder(program_point);

    auto &encoded = add_value(builder, *used);

    uses.value(encoded);

    for (auto *use : uses) {
      update_use(*use, encoded);
    }
  }

  for (auto &uses : decoded) {

    // Unpack the use.
    auto *use = uses.front();
    auto *used = use->get();
    auto *user = dyn_cast<llvm::Instruction>(use->getUser());

    // Compute the insertion point.
    llvm::Instruction *program_point = nullptr;
    if (user) {
      if (auto *phi = dyn_cast<llvm::PHINode>(user)) {
        auto *incoming_block = phi->getIncomingBlock(*use);
        // TODO: this may be unsound if the terminator is conditional.
        program_point = incoming_block->getTerminator();
      } else {
        program_point = user;
      }

    } else {
      MEMOIR_UNREACHABLE("Failed to find a point to decode the value!");
    }

    MemOIRBuilder builder(program_point);

    auto &decoded = decode_value(builder, *used);

    uses.value(decoded);

    for (auto *use : uses) {
      use->set(&decoded);
    }
  }

  return;
}

bool value_will_be_inserted(llvm::Value &value, InsertInst &insert) {

  auto *inst = dyn_cast<llvm::Instruction>(&value);
  llvm::BasicBlock *value_bb = nullptr;
  if (inst) {
    value_bb = inst->getParent();
  } else if (auto *arg = dyn_cast<llvm::Argument>(&value)) {
    auto *func = arg->getParent();
    value_bb = &func->getEntryBlock();
  } else {
    MEMOIR_UNREACHABLE("Unhandled value: ", value);
  }

  auto *user_bb = insert.getParent();

  // If the value and user are in the same basic block, then it will
  // definitely be used.
  if (value_bb == user_bb) {
    return true;
  }

  // If the use postdominates the value definition.
  auto *func = user_bb->getParent();

  // Check if the every path from the value must include the insert
  // instruction, a has instruction equivalent to the inserted key, or is
  // unreachable.
  bool will_be_inserted = true;
  WorkList<llvm::BasicBlock *, /* VisitOnce? */ true> worklist = { value_bb };
  while (not worklist.empty()) {
    auto *bb = worklist.pop();

    // If we hit the insert, then this path hits the insert.
    if (bb == user_bb) {
      continue;
    }

    auto *terminator = bb->getTerminator();
    if (isa<llvm::UnreachableInst>(terminator)) {
      // If this path is unreachable, then yippee!
      continue;
    } else if (isa<llvm::ReturnInst>(terminator)) {
      // If we found a return, then we didn't insert along this path!
      will_be_inserted = false;
      break;
    } else if (auto *branch = dyn_cast<llvm::BranchInst>(terminator)) {
      // Check if this branch checks if we have already inserted this value.
      if (branch->isConditional()) {
        if (auto *cond = branch->getCondition()) {
          if (auto *has = into<HasInst>(cond)) {
            // Check if the offsets match.
            if (&insert.getObject() == &has->getObject()
                and std::equal(insert.indices_begin(),
                               insert.indices_end(),
                               has->indices_begin(),
                               has->indices_end())) {
              // Enqueue the false branch.
              worklist.push(branch->getSuccessor(1));
              continue;
            }
          }
        }
      }

      // Enqueue all successors.
      for (auto *succ : branch->successors()) {
        worklist.push(succ);
      }
    }
  }

  if (will_be_inserted) {
    return true;
  }

  // Otherwise, we can't guarantee the value will be present.
  return false;
}

bool is_total_proxy(ObjectInfo &info, const Vector<CoalescedUses> &added) {

  println();
  println("TOTAL PROXY? ", info);

  // Check that the object is not a propagator.
  // NOTE: this is conservative, but the check for a propagator to be a total
  // proxt is difficult.
  if (info.is_propagator()) {
    println("NO, propagator");
    return false;
  }

  // Check that there is guaranteed to be one of these objects for each
  // allocation.
  for (auto offset : info.offsets) {
    if (offset == unsigned(-1)) {
      println("NO, not singular");
      return false;
    }
  }

  // Check that this value is never cleared or removed from.
  bool monotonic = true;
  for (const auto &[func, base_to_redefs] : info.redefinitions) {
    for (const auto &[base, redefs] : base_to_redefs) {
      for (const auto &redef : redefs) {
        auto *value = &redef.value();
        if (into<RemoveInst>(value) or into<ClearInst>(value)) {
          println("NO, keys are removed");
          return false;
        }
      }
    }
  }

  // Check that for each addkey use, this object is inserted into.
  // Also, ensure that the encoded value is in a control flow equivalent block
  // to the uses.
  Set<llvm::Value *> values_added = {};
  Set<llvm::Value *> values_needed = {};
  for (auto &uses : added) {
    auto &value = uses.value();
    auto &encoded =
        MEMOIR_SANITIZE(dyn_cast<llvm::Instruction>(&value),
                        "Encoded value ",
                        value,
                        " is not an instruction! Something went wrong.");

    values_needed.insert(&value);

    bool found_use = false;
    for (auto *use : uses) {
      auto *user = use->getUser();

      if (auto *access = into<AccessInst>(user)) {

        // Skip irrelevant accesses.
        if (not info.is_redefinition(access->getObject())) {
          continue;
        }

        // Check that this access is at the correct offset.
        auto distance = access->match_offsets(info.offsets);
        if (not distance) {
          continue;
        }

        if (distance.value() < info.offsets.size()) {
          println("NO, wrong offset in ", *access);
          return false;
        }

        // Ensure that this access is control equivalent to the encoded value.
        if (auto *insert = dyn_cast<InsertInst>(access)) {
          if (not value_will_be_inserted(value, *insert)) {
            println("NO, not control equivalent");
            return false;
          }
        }

        // Otherwise, we found a valid use.
        found_use = true;

        values_added.insert(&value);

      } else {
        // Skip non-accesses.
        continue;
      }
    }

    // If we failed to find a use, then the check fails.
#if 0
    if (not found_use) {
      println("NO, failed to find use of addkey for: ");
      println("  ", value);
    }
#endif
  }

  // Check if we have added all needed values.
  auto added_all_needed =
      std::accumulate(values_needed.begin(),
                      values_needed.end(),
                      true,
                      [&](bool needed, llvm::Value *val) {
                        return needed and values_added.count(val);
                      });
  if (not added_all_needed) {
    println("NO, did not add all needed values.");
    return false;
  }

  // If we got this far, then we're good to go!
  println("YES!");
  return true;
}

Type &convert_to_sequence_type(Type &base, llvm::ArrayRef<unsigned> offsets) {

  infoln("CONVERT TO SEQUENCE:");
  infoln(base);
  for (auto offset : offsets) {
    info(offset, ", ");
  }
  infoln();

  if (auto *tuple_type = dyn_cast<TupleType>(&base)) {

    Vector<Type *> fields = tuple_type->fields();

    auto field = offsets[0];

    fields[field] = &convert_to_sequence_type(tuple_type->getFieldType(field),
                                              offsets.drop_front());

    return TupleType::get(fields);

  } else if (auto *seq_type = dyn_cast<SequenceType>(&base)) {

    return SequenceType::get(
        convert_to_sequence_type(seq_type->getElementType(),
                                 offsets.drop_front()),
        seq_type->get_selection());

  } else if (auto *assoc_type = dyn_cast<AssocType>(&base)) {

    // If the offsets are empty, replace the keys.
    if (offsets.empty()) {
      auto &converted = SequenceType::get(assoc_type->getValueType());
      infoln("CONVERTED: ", converted);
      return converted;
    }

    return AssocType::get(assoc_type->getKeyType(),
                          convert_to_sequence_type(assoc_type->getValueType(),
                                                   offsets.drop_front()),
                          assoc_type->get_selection());

  } else if (offsets.empty()) {
    return base;
  }

  MEMOIR_UNREACHABLE("Failed to convert type!");
}

Type &convert_element_type(Type &base,
                           llvm::ArrayRef<unsigned> offsets,
                           Type &new_type,
                           bool is_nested = false) {

  if (auto *tuple_type = dyn_cast<TupleType>(&base)) {

    Vector<Type *> fields = tuple_type->fields();

    auto field = offsets[0];

    fields[field] = &convert_element_type(tuple_type->getFieldType(field),
                                          offsets.drop_front(),
                                          new_type,
                                          is_nested);

    return TupleType::get(fields);

  } else if (auto *seq_type = dyn_cast<SequenceType>(&base)) {

    return SequenceType::get(convert_element_type(seq_type->getElementType(),
                                                  offsets.drop_front(),
                                                  new_type,
                                                  true),
                             seq_type->get_selection());

  } else if (auto *assoc_type = dyn_cast<AssocType>(&base)) {

    // If the offsets are empty, replace the keys.
    if (offsets.empty()) {

      auto selection = assoc_type->get_selection();
      if (not selection) {
        if (isa<VoidType>(&assoc_type->getValueType())) {
          selection = is_nested ? proxy_nested_set_impl : proxy_set_impl;
        } else {
          selection = proxy_map_impl;
        }
      }

      return AssocType::get(new_type, assoc_type->getValueType(), selection);
    }

    return AssocType::get(assoc_type->getKeyType(),
                          convert_element_type(assoc_type->getValueType(),
                                               offsets.drop_front(),
                                               new_type,
                                               true),
                          assoc_type->get_selection());

  } else if (offsets.empty()) {
    return new_type;
  }

  MEMOIR_UNREACHABLE("Failed to convert type!");
}

static Map<llvm::Function *, Set<llvm::Value *>> gather_candidate_uses(
    Candidate &candidate,
    Set<llvm::Use *> &to_decode,
    Set<llvm::Use *> &to_encode,
    Set<llvm::Use *> &to_addkey) {

  Map<llvm::Function *, Set<llvm::Value *>> encoded = {};
  for (auto *info : candidate) {
    for (const auto &[func, values] : info->encoded) {
      encoded[func].insert(values.begin(), values.end());
    }

    for (const auto &[func, uses] : info->to_encode) {
      to_encode.insert(uses.begin(), uses.end());
    }

    for (const auto &[func, uses] : info->to_addkey) {
      to_addkey.insert(uses.begin(), uses.end());
    }
  }

  // Perform a forward analysis on the encoded values.
  forward_analysis(encoded);

  // Collect the set of uses to decode.
  for (const auto &[func, values] : encoded) {
    for (auto *val : values) {
      for (auto &use : val->uses()) {
        auto *user = use.getUser();

        // If the user is a PHI/Select/Fold _and_ is also encoded, we don't
        // need to decode it.
        if (isa<llvm::PHINode>(user) or isa<llvm::SelectInst>(user)) {
          if (encoded[func].count(user)) {
            continue;
          }
        } else if (auto *fold = into<FoldInst>(user)) {
          auto *arg = (&use == &fold->getInitialAsUse())
                          ? &fold->getAccumulatorArgument()
                          : fold->getClosedArgument(use);
          if (encoded[&fold->getBody()].count(arg)) {
            continue;
          }
        } else if (auto *ret = dyn_cast<llvm::ReturnInst>(user)) {
          if (auto *func = ret->getFunction()) {
            if (auto *fold = FoldInst::get_single_fold(*func)) {
              auto *caller = fold->getFunction();
              auto &result = fold->getResult();
              // If the result of the fold is encoded, we don't decode here.
              if (encoded[caller].count(&result)) {
                continue;
              }
            }
          }
        } else if (auto *call = dyn_cast<llvm::CallBase>(user)) {
          // TODO
        }

        // If the user is not an encoded propagator, we need to decode this
        // use.
        to_decode.insert(&use);
      }
    }
  }

  return encoded;
}

static void print_uses(const Set<llvm::Use *> &to_encode,
                       const Set<llvm::Use *> &to_decode,
                       const Set<llvm::Use *> &to_addkey) {
  println("    ", to_encode.size(), " USES TO ENCODE ");
  for (auto *use : to_encode) {
    infoln(pretty_use(*use));
  }
  infoln();
  println("    ", to_decode.size(), " USES TO DECODE ");
  for (auto *use : to_decode) {
    infoln(pretty_use(*use));
  }
  infoln();
  println("    ", to_addkey.size(), " USES TO ADDKEY ");
  for (auto *use : to_addkey) {
    infoln(pretty_use(*use));
  }
}

bool ProxyInsertion::transform() {

  bool modified = false;

  // Transform the program for each candidate.
  for (auto candidates_it = this->candidates.begin();
       candidates_it != this->candidates.end();
       ++candidates_it) {

    modified |= true;

    auto &candidate = *candidates_it;

    infoln();
    infoln("PROXYING CANDIDATE:");
    for (const auto *candidate_info : candidate) {
      infoln("  ", *candidate_info);
    }

    // Unpack the first object information.
    auto *first_info = candidate.front();
    auto *alloc = first_info->allocation;
    auto *type = &alloc->getType();

    // Get the nested object type.
    for (auto offset : first_info->offsets) {
      if (auto *tuple_type = dyn_cast<TupleType>(type)) {
        type = &tuple_type->getFieldType(offset);
      } else if (auto *collection_type = dyn_cast<CollectionType>(type)) {
        type = &collection_type->getElementType();
      } else {
        MEMOIR_UNREACHABLE("Invalid offsets provided.");
      }
    }

    auto &assoc_type = MEMOIR_SANITIZE(
        dyn_cast<AssocType>(type),
        "Proxy insertion for non-assoc collection is unsupported");

    auto &key_type = assoc_type.getKeyType();
    auto &val_type = assoc_type.getValueType();

    // Collect all of the uses that need to be handled.
    Set<llvm::Use *> to_decode = {};
    Set<llvm::Use *> to_encode = {};
    Set<llvm::Use *> to_addkey = {};
    auto encoded_values =
        gather_candidate_uses(candidate, to_decode, to_encode, to_addkey);

    println("  FOUND USES");
    print_uses(to_encode, to_decode, to_addkey);

    if (not disable_use_weakening) {
      // DISABLED: Use weakening works, but does not have any considerable
      // performance benefits.

      // Weaken uses from addkey to encode if we know that the value is already
      // inserted.
      Set<llvm::Use *> to_weaken = {};
      weaken_uses(to_addkey, to_weaken, candidate, this->get_bounds_checks);
      for (auto *use : to_weaken) {
        to_addkey.erase(use);
        to_encode.insert(use);
      }
    }

    if (not disable_translation_elimination) {
      // Trim uses that dont need to be decoded because they are only used to
      // compare against other values that need to be decoded.
      Set<llvm::Use *> trim_to_decode = {};
      for (auto *use : to_decode) {
        auto *user = use->getUser();

        if (auto *cmp = dyn_cast<llvm::CmpInst>(user)) {
          if (cmp->isEquality()) {
            auto &lhs = cmp->getOperandUse(0);
            auto &rhs = cmp->getOperandUse(1);
            if (used_value_will_be_decoded(lhs, to_decode)
                and used_value_will_be_decoded(rhs, to_decode)) {
              trim_to_decode.insert(&lhs);
              trim_to_decode.insert(&rhs);
            }
          }
        }
      }

      // Trim uses that dont need to be encoded because they are produced by a
      // use that needs decoded.
      Set<llvm::Use *> trim_to_encode = {};
      for (auto uses : { to_encode, to_addkey }) {
        for (auto *use : uses) {
          if (used_value_will_be_decoded(*use, to_decode)) {
            trim_to_encode.insert(use);
            trim_to_decode.insert(use);
          }
        }
      }

      // Erase the uses that we identified to trim.
      for (auto *use_to_trim : trim_to_decode) {
        to_decode.erase(use_to_trim);
      }
      for (auto *use_to_trim : trim_to_encode) {
        to_encode.erase(use_to_trim);
        to_addkey.erase(use_to_trim);
      }
    }

    println("  TRIMMED USES:");
    print_uses(to_encode, to_decode, to_addkey);

    // Find the construction point for the encoder and decoder.
    llvm::Instruction *construction_point = &alloc->getCallInst();

    // Find a point that dominates all of the object allocations.
    auto *construction_function = construction_point->getFunction();
    auto &DT = this->get_dominator_tree(*construction_function);
    for (const auto *other : candidate) {
      auto *other_alloc = other->allocation;
      auto &other_inst = other_alloc->getCallInst();

      construction_point =
          DT.findNearestCommonDominator(construction_point, &other_inst);
    }

    // Fetch LLVM context.
    auto &context = construction_point->getContext();
    auto &module = MEMOIR_SANITIZE(construction_point->getModule(),
                                   "Construction point has no module.");
    auto &data_layout = module.getDataLayout();

    // Allocate the proxy.
    MemOIRBuilder builder(construction_point);

    auto &size_type = Type::get_size_type(data_layout);
    auto &llvm_size_type = *size_type.get_llvm_type(context);
    auto size_type_bitwidth = size_type.getBitWidth();

    // Determine which proxies we need.
    bool build_encoder = to_encode.size() > 0 or to_addkey.size() > 0;
    bool build_decoder = to_decode.size() > 0;

    llvm::Instruction *encoder = nullptr;
    Type *encoder_type = nullptr;
    if (build_encoder) {
      encoder_type = &AssocType::get(key_type, size_type);
      auto *encoder_alloc =
          builder.CreateAllocInst(*encoder_type, {}, "proxy.encode.");
      encoder = &encoder_alloc->getCallInst();
    }

    llvm::Instruction *decoder = nullptr;
    Type *decoder_type = nullptr;
    if (build_decoder) {
      decoder_type = &SequenceType::get(key_type);
      auto *decoder_alloc = builder.CreateAllocInst(*decoder_type,
                                                    { builder.getInt64(0) },
                                                    "proxy.decode.");
      decoder = &decoder_alloc->getCallInst();
    }

    // Make the proxy available at all uses.
    // TODO: replace this with the new SSA repair utility.
    Map<llvm::Function *, llvm::Instruction *> function_to_encoder = {
      { NULL, encoder }
    };
    if (build_encoder) {
      add_tempargs(function_to_encoder,
                   { to_encode, to_addkey },
                   *encoder,
                   *encoder_type,
                   "encoder.");
    }

    Map<llvm::Function *, llvm::Instruction *> function_to_decoder = {
      { NULL, decoder }
    };
    if (build_decoder) {
      add_tempargs(function_to_decoder,
                   { to_decode, to_addkey },
                   *decoder,
                   *decoder_type,
                   "decoder.");
    }

    // Create the addkey function.
    auto addkey_callee = create_addkey_function(this->M,
                                                key_type,
                                                build_encoder,
                                                encoder_type,
                                                build_decoder,
                                                decoder_type);

    // Coalesce the values that have been encoded/decoded.
    auto decoded = coalesce(to_decode, this->get_dominator_tree);
    auto encoded = coalesce(to_encode, this->get_dominator_tree);
    auto added = coalesce(to_addkey, this->get_dominator_tree);

    // Report the coalescing.
    println("  AFTER COALESCING:");
    println("    USES TO ENCODE ", encoded.size());
    println("    USES TO DECODE ", decoded.size());
    println("    USES TO ADDKEY ", added.size());

    // Create anon functions to encode/decode a value
    std::function<llvm::Value &(llvm::Value &)> get_encoder =
        [&](llvm::Value &value) -> llvm::Value & {
      auto *function = parent_function(value);
      MEMOIR_ASSERT(function, "Failed to find parent function for ", value);
      return MEMOIR_SANITIZE(function_to_encoder[function],
                             "Failed to find encoder in ",
                             function->getName());
    };

    std::function<llvm::Value &(MemOIRBuilder &, llvm::Value &)> decode_value =
        [&](MemOIRBuilder &builder, llvm::Value &value) -> llvm::Value & {
      auto *function = parent_function(value);
      if (not function) {
        if (auto *basic_block = builder.GetInsertBlock()) {
          function = basic_block->getParent();
        }
      }
      MEMOIR_ASSERT(function, "Failed to find parent function for ", value);

      auto *decoder = function_to_decoder.at(function);
      return builder.CreateReadInst(key_type, decoder, { &value })->asValue();
    };

    std::function<llvm::Value &(MemOIRBuilder &, llvm::Value &)> encode_value =
        [&](MemOIRBuilder &builder, llvm::Value &value) -> llvm::Value & {
      auto *function = parent_function(value);
      if (not function) {
        if (auto *basic_block = builder.GetInsertBlock()) {
          function = basic_block->getParent();
        }
      }
      MEMOIR_ASSERT(function, "Failed to find parent function for ", value);

      auto *encoder = function_to_encoder.at(function);
      return builder.CreateReadInst(size_type, encoder, &value)->asValue();
    };

    std::function<llvm::Value &(MemOIRBuilder &, llvm::Value &)> add_value =
        [&](MemOIRBuilder &builder, llvm::Value &value) -> llvm::Value & {
      auto *function = parent_function(value);
      if (not function) {
        if (auto *basic_block = builder.GetInsertBlock()) {
          function = basic_block->getParent();
        }
      }
      MEMOIR_ASSERT(function, "Failed to find parent function for ", value);
      Vector<llvm::Value *> args = { &value };
      if (build_encoder) {
        auto *encoder = function_to_encoder.at(function);
        args.push_back(encoder);
      }
      if (build_decoder) {
        auto *decoder = function_to_decoder.at(function);
        args.push_back(decoder);
      }
      return MEMOIR_SANITIZE(builder.CreateCall(addkey_callee, args),
                             "Failed to create call to addkey!");
    };

    // Inject instructions to handle each use.
    inject(context,
           decoded,
           encoded,
           added,
           get_encoder,
           decode_value,
           encode_value,
           add_value);

    // Collect any function parameters whose type has changed because the
    // argument propagates an encoded value.
    // TODO: make this monomorphize the functions as well.
    OrderedMultiMap<llvm::Function *, llvm::Argument *> params_to_mutate = {};
    for (const auto &[func, values] : encoded_values) {
      for (auto *val : values) {
        if (auto *arg = dyn_cast<llvm::Argument>(val)) {
          if (arg->getType() != &llvm_size_type) {
            params_to_mutate.emplace(arg->getParent(), arg);
          }
        }
      }
    }

    // Mutate the type of function parameters in the program.
    Set<llvm::Function *> to_cleanup = {};
    for (auto it = params_to_mutate.begin(); it != params_to_mutate.end();) {
      auto *func = it->first;

      auto *module = func->getParent();
      auto &data_layout = module->getDataLayout();

      auto *func_type = func->getFunctionType();

      // Collect the original parameter types.
      Vector<llvm::Type *> param_types(func_type->param_begin(),
                                       func_type->param_end());

      // Update the parameter types that are encoded.
      for (; it != params_to_mutate.upper_bound(func); ++it) {
        auto *arg = it->second;
        auto arg_idx = arg->getArgNo();

        param_types[arg_idx] = &llvm_size_type;
      }

      // Create the new function type.
      auto *new_func_type = llvm::FunctionType::get(func_type->getReturnType(),
                                                    param_types,
                                                    func_type->isVarArg());

      // TODO: Fix the attributes on the new function.

      // Create the empty function to clone into.
      auto &new_func =
          MEMOIR_SANITIZE(llvm::Function::Create(new_func_type,
                                                 func->getLinkage(),
                                                 func->getName(),
                                                 module),
                          "Failed to create new function.");

      debugln("CLONING ", func->getName());

      // Update the function to the new type.
      llvm::ValueToValueMapTy vmap;
      for (auto &old_arg : func->args()) {
        auto *new_arg = new_func.getArg(old_arg.getArgNo());
        vmap.insert({ &old_arg, new_arg });
      }
      llvm::SmallVector<llvm::ReturnInst *, 8> returns;
      llvm::CloneFunctionInto(&new_func,
                              func,
                              vmap,
                              llvm::CloneFunctionChangeType::LocalChangesOnly,
                              returns);

      new_func.takeName(func);
      func->replaceAllUsesWith(&new_func);

      // Update the object info.
      for (auto it = candidates_it; it != candidates.end(); ++it) {
        for (auto &info : *it) {
          info->update(*func,
                       new_func,
                       vmap,
                       /* delete old? */ true);
        }
      }

      // Clean up the old function.
      to_cleanup.insert(func);
    }

    // Collect the types that each candidate needs to be mutated to.
    AssocList<AllocInst *, Type *> types_to_mutate;
    for (auto *info : candidate) {
      auto *alloc = info->allocation;

      // Initialize the type to mutate if it doesnt exist already.
      auto found = types_to_mutate.find(alloc);
      if (found == types_to_mutate.end()) {
        types_to_mutate[alloc] = &alloc->getType();
      }
      auto &type = types_to_mutate[alloc];

      // If the object is a total proxy, update it to be a sequence.
      if (is_total_proxy(*info, added) and not disable_total_proxy) {

        println(Style::BOLD, Colors::GREEN, "FOUND TOTAL PROXY", Style::RESET);

        type = &convert_to_sequence_type(*type, info->offsets);

      } else {
        // Convert the type at the given offset to the size type.
        type = &convert_element_type(*type, info->offsets, size_type);
      }
    }

    // Mutate the types in the program.
    auto mutate_it = types_to_mutate.begin(), mutate_ie = types_to_mutate.end();
    for (; mutate_it != mutate_ie; ++mutate_it) {
      auto *alloc = mutate_it->first;
      auto *type = mutate_it->second;

      // Mutate the type.
      mutate_type(
          *alloc,
          *type,
          [&](llvm::Function &old_func,
              llvm::Function &new_func,
              llvm::ValueToValueMapTy &vmap) {
            // Update the remaining objects in the candidate.
            for (auto it = std::next(candidates_it); it != candidates.end();
                 ++it) {
              for (auto &info : *it) {
                info->update(old_func,
                             new_func,
                             vmap,
                             /* delete old? */ true);
              }
            }

            // Update allocations marked for mutation.
            for (auto it = std::next(mutate_it); it != mutate_ie; ++it) {
              auto *alloc = it->first;
              auto &inst = alloc->getCallInst();

              if (inst.getFunction() == &old_func) {
                auto *new_inst = &*vmap[&inst];
                auto *new_alloc = into<AllocInst>(new_inst);

                // Update in-place.
                it->first = new_alloc;
              }
            }
          });
    }

    for (auto *func : to_cleanup) {
      func->deleteBody();
      func->eraseFromParent();
    }
  }

  return modified;
}

ProxyInsertion::ProxyInsertion(llvm::Module &M,
                               GetDominatorTree get_dominator_tree,
                               GetBoundsChecks get_bounds_checks)
  : M(M),
    get_dominator_tree(get_dominator_tree),
    get_bounds_checks(get_bounds_checks) {

  // Register the bit{map,set} implementations.
  Implementation::define({
      Implementation(proxy_set_impl,
                     AssocType::get(TypeVariable::get(), VoidType::get()),
                     /* selectable? */ false),

      Implementation(proxy_map_impl,
                     AssocType::get(TypeVariable::get(), TypeVariable::get()),
                     /* selectable? */ false),

      Implementation("bitset",
                     AssocType::get(TypeVariable::get(), VoidType::get()),
                     /* selectable? */ false),

      Implementation("bitmap",
                     AssocType::get(TypeVariable::get(), TypeVariable::get()),
                     /* selectable? */ false),

      Implementation("sparse_bitset",
                     AssocType::get(TypeVariable::get(), VoidType::get()),
                     /* selectable? */ false),

      Implementation("sparse_bitmap",
                     AssocType::get(TypeVariable::get(), TypeVariable::get()),
                     /* selectable? */ false),

      Implementation("twined_bitmap",
                     AssocType::get(TypeVariable::get(), TypeVariable::get()),
                     /* selectable? */ false),

  });

  analyze();

  transform();

  for (auto &F : M) {
    if (not F.empty()) {
      if (llvm::verifyFunction(F, &llvm::errs())) {
        println(F);
        MEMOIR_UNREACHABLE("Failed to verify ", F.getName());
      }
    }
  }

  MemOIRInst::invalidate();

  reify_tempargs(M);

  for (auto &F : M) {
    if (not F.empty()) {
      if (llvm::verifyFunction(F, &llvm::errs())) {
        println(F);
        MEMOIR_UNREACHABLE("Failed to verify ", F.getName());
      }
    }
  }
}

} // namespace folio
