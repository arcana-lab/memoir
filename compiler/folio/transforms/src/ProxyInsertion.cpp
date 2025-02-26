#include "llvm/IR/AttributeMask.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "memoir/ir/Builder.hpp"
#include "memoir/lowering/Implementation.hpp"
#include "memoir/transforms/utilities/ReifyTempArgs.hpp"
#include "memoir/utility/Metadata.hpp"

#include "folio/transforms/ProxyInsertion.hpp"

using namespace llvm::memoir;

namespace folio {

static void update_values(llvm::ValueToValueMapTy &vmap,
                          set<llvm::Value *> &input,
                          set<llvm::Value *> &output) {
  for (auto *val : input) {
    auto *clone = &*vmap[val];

    output.insert(clone);
  }
}

static void update_uses(map<llvm::Function *, set<llvm::Use *>> &uses,
                        llvm::Function &old_func,
                        llvm::Function &new_func,
                        llvm::ValueToValueMapTy &vmap,
                        bool delete_old) {

  if (uses.count(&old_func)) {

    auto &old_uses = uses[&old_func];
    auto &new_uses = uses[&new_func];

    if (delete_old) {
      uses.erase(&old_func);
    }

    for (auto *use : old_uses) {
      auto *user = dyn_cast<llvm::Instruction>(use->getUser());
      auto *clone = dyn_cast<llvm::Instruction>(&*vmap[user]);

      auto &clone_use = clone->getOperandUse(use->getOperandNo());

      new_uses.insert(&clone_use);
    }
  }
}

void ObjectInfo::update(llvm::Function &old_func,
                        llvm::Function &new_func,
                        llvm::ValueToValueMapTy &vmap,
                        bool delete_old) {

  // Unpack the object info.
  auto *alloc = this->allocation;
  auto &inst = alloc->getCallInst();

  // If this allocations parent is the function being cloned, we need
  // to update it.
  if (inst.getFunction() == &old_func) {
    auto *new_inst = &*vmap[&inst];
    auto *new_alloc = into<AllocInst>(new_inst);

    // Update in-place.
    this->allocation = new_alloc;
  }

  // Update the set of redefinitions.
  auto &redefs = this->redefinitions;
  if (redefs.count(&old_func)) {

    update_values(vmap, redefs[&old_func], redefs[&new_func]);

    if (delete_old) {
      redefs.erase(&old_func);
    }
  }

  // Update the uses.
  update_uses(this->to_encode, old_func, new_func, vmap, delete_old);
  update_uses(this->to_decode, old_func, new_func, vmap, delete_old);
  update_uses(this->to_addkey, old_func, new_func, vmap, delete_old);

  return;
}

uint32_t ObjectInfo::compute_heuristic(const ObjectInfo &other) const {
  uint32_t benefit = 0;
  for (const auto &[func, decode_uses] : this->to_decode) {
    for (const auto *use_to_decode : decode_uses) {
      if (other.to_encode.count(func) > 0) {
        for (const auto *use_to_encode : other.to_encode.at(func)) {
          if (use_to_decode == use_to_encode) {
            ++benefit;
          }
        }
      }

      if (other.to_addkey.count(func) > 0) {
        for (const auto *use_to_addkey : other.to_addkey.at(func)) {
          if (use_to_decode == use_to_addkey) {
            ++benefit;
          }
        }
      }

      if (other.to_decode.count(func) > 0) {
        auto *cmp = dyn_cast<llvm::CmpInst>(use_to_decode->getUser());
        if (cmp and cmp->isEquality()) {
          if (other.to_decode.at(func).count(&cmp->getOperandUse(0))) {
            ++benefit;
          } else if (other.to_decode.at(func).count(&cmp->getOperandUse(1))) {
            ++benefit;
          }
        }
      }
    }
  }

  return benefit;
}

static void gather_redefinitions(
    llvm::Value &V,
    map<llvm::Function *, set<llvm::Value *>> &redefinitions) {

  llvm::Function *function = nullptr;

  if (auto *inst = dyn_cast<llvm::Instruction>(&V)) {
    function = inst->getFunction();
  } else if (auto *arg = dyn_cast<llvm::Argument>(&V)) {
    function = arg->getParent();
  }

  MEMOIR_ASSERT(function, "Unknown parent function for redefinition.");

  if (redefinitions[function].count(&V) > 0) {
    return;
  }

  redefinitions[function].insert(&V);

  for (auto &use : V.uses()) {
    auto *user = use.getUser();

    // Recurse on redefinitions.
    if (auto *phi = dyn_cast<llvm::PHINode>(user)) {
      gather_redefinitions(*user, redefinitions);

    } else if (auto *memoir_inst = into<MemOIRInst>(user)) {
      if (auto *update = dyn_cast<UpdateInst>(memoir_inst)) {
        if (&use == &update->getObjectAsUse()) {
          gather_redefinitions(*user, redefinitions);
        }

      } else if (isa<RetPHIInst>(memoir_inst) or isa<UsePHIInst>(memoir_inst)) {

        // Recurse on redefinitions.
        gather_redefinitions(*user, redefinitions);
      }

      // Gather variable if folded on, or recurse on closed argument.
      else if (auto *fold = into<FoldInst>(user)) {

        if (use == fold->getInitialAsUse()) {
          // Gather uses of the accumulator argument.
          gather_redefinitions(fold->getAccumulatorArgument(), redefinitions);

          // Gather uses of the resultant.
          gather_redefinitions(fold->getResult(), redefinitions);

        } else if (use == fold->getObjectAsUse()) {
          // Do nothing.

        } else if (auto *closed_arg = fold->getClosedArgument(use)) {
          // Gather uses of the closed argument.
          gather_redefinitions(*closed_arg, redefinitions);
        }
      }

    } else if (auto *call = dyn_cast<llvm::CallBase>(user)) {
      auto &callee =
          MEMOIR_SANITIZE(call->getCalledFunction(),
                          "Found use of MEMOIR collection by indirect callee!");

      auto &arg =
          MEMOIR_SANITIZE(callee.getArg(use.getOperandNo()),
                          "No arguments in the callee matching this use!");

      gather_redefinitions(arg, redefinitions);
    }
  }

  return;
}

namespace detail {

static bool is_last_index(llvm::Use *use,
                          AccessInst::index_op_iterator index_end) {
  return std::next(AccessInst::index_op_iterator(use)) == index_end;
}

static llvm::Use *get_index_use(AccessInst &access, vector<unsigned> &offsets) {
  auto offset_it = offsets.begin();
  auto offset_ie = offsets.end();

  auto index_it = access.index_operands_begin();
  auto index_ie = access.index_operands_end();

  auto *type = &access.getObjectType();

  for (auto offset : offsets) {

    // If we have reached the end of the index operands, there is no index
    // use.
    if (index_it == index_ie) {
      return nullptr;
    }

    if (auto *struct_type = dyn_cast<StructType>(type)) {

      auto &index_use = *index_it;
      auto &index_const =
          MEMOIR_SANITIZE(dyn_cast<llvm::ConstantInt>(index_use.get()),
                          "Field index is not statically known!");

      // If the offset doesn't match the field index, there is no index use.
      if (offset != index_const.getZExtValue()) {
        return index_ie;
      }

      // Get the inner type.
      type = &struct_type->getFieldType(offset);

    } else if (auto *collection_type = dyn_cast<CollectionType>(type)) {
      // Get the inner type.
      type = &collection_type->getElementType();
    }

    ++index_it;
  }

  // If we are at the end of the index operands, return NULL.
  if (index_it == index_ie) {
    return nullptr;
  }

  // Otherwise, fetch the index use and return it.
  auto *index_use = &*index_it;

  return index_it;
}

static int32_t indices_match_offsets(AccessInst &access,
                                     llvm::ArrayRef<unsigned> offsets) {
  auto index_it = access.index_operands_begin();
  auto index_ie = access.index_operands_end();

  auto *object_type = type_of(access.getObject());
  if (not object_type) {
    println(*access.getParent()->getParent());
    println("OBJ ", access.getObject());
    MEMOIR_UNREACHABLE("Could not get type of object ", access);
  }

  auto *type = &access.getObjectType();

  auto offset_it = offsets.begin();
  for (; offset_it != offsets.end(); ++offset_it) {

    auto offset = *offset_it;

    // If we have reached the end of the index operands, there is no index
    // use.
    if (index_it == index_ie) {
      break;
    }

    if (auto *struct_type = dyn_cast<StructType>(type)) {

      auto &index_use = *index_it;
      auto &index_const =
          MEMOIR_SANITIZE(dyn_cast<llvm::ConstantInt>(index_use.get()),
                          "Field index is not statically known!");

      // If the offset doesn't match the field index, there is no index use.
      if (offset != index_const.getZExtValue()) {
        return -1;
      }

      // Get the inner type.
      type = &struct_type->getFieldType(offset);

    } else if (auto *collection_type = dyn_cast<CollectionType>(type)) {
      // Get the inner type.
      type = &collection_type->getElementType();
    }

    ++index_it;
  }

  // Return the number of offsets we matched.
  return std::distance(offsets.begin(), offset_it);
}

} // namespace detail

static void gather_uses_to_proxy(
    llvm::Value &V,
    vector<unsigned> &offsets,
    map<llvm::Function *, set<llvm::Use *>> &to_encode,
    map<llvm::Function *, set<llvm::Use *>> &to_decode,
    map<llvm::Function *, set<llvm::Use *>> &to_addkey) {

  infoln("REDEF ", V);

  llvm::Function *function = nullptr;

  if (auto *inst = dyn_cast<llvm::Instruction>(&V)) {
    function = inst->getFunction();
  } else if (auto *arg = dyn_cast<llvm::Argument>(&V)) {
    function = arg->getParent();
  }

  MEMOIR_ASSERT(function, "Gathering uses of value with no parent function!");

  // From a given collection, V, gather all uses that need to be either
  // encoded or decoded.
  for (auto &use : V.uses()) {
    auto *user = use.getUser();

    infoln("  USER ", *user);

    if (auto *fold = into<FoldInst>(user)) {

      if (use == fold->getObjectAsUse()) {

        // If we find an index use, encode it.
        if (auto *index_use = detail::get_index_use(*fold, offsets)) {
          infoln("    ENCODING INDEX");
          to_encode[function].insert(index_use);

        } else {

          // If the offset is exactly equal to the keys being folded over,
          // decode the index argument of the body.
          auto distance = detail::indices_match_offsets(*fold, offsets);

          if (distance == -1) {
            // Do nothing.
          }

          // If the offsets are fully exhausted, add uses of the index
          // argument to the set of uses to decode.
          else if (distance == int32_t(offsets.size())) {
            auto &index_arg = fold->getIndexArgument();
            infoln("    DECODING KEY");
            for (auto &index_use : index_arg.uses()) {
              infoln("      USE ", *index_use.getUser());
              to_decode[&fold->getBody()].insert(&index_use);
            }
          }

          // If the offsets are not fully exhausted, recurse on the value
          // argument.
          else if (distance < int32_t(offsets.size())) {
            if (auto *elem_arg = fold->getElementArgument()) {
              vector<unsigned> nested_offsets(
                  std::next(offsets.begin(), distance + 1),
                  offsets.end());
              infoln("    RECURSING");
              gather_uses_to_proxy(*elem_arg,
                                   nested_offsets,
                                   to_encode,
                                   to_decode,
                                   to_addkey);
            }
          }
        }
      }

    } else if (auto *access = into<AccessInst>(user)) {

      if (use == access->getObjectAsUse()) {
        // Find the index use for the given offset and mark it for decoding.
        if (auto *index_use = detail::get_index_use(*access, offsets)) {
          if (isa<InsertInst>(access)) {
            if (detail::is_last_index(index_use,
                                      access->index_operands_end())) {
              infoln("    ADDING KEY ", *index_use->get());
              to_addkey[function].insert(index_use);
              continue;
            }
          }

          infoln("    ENCODING KEY ", *index_use->get());
          to_encode[function].insert(index_use);
        }
      }
    }
  }

  return;
}

void ObjectInfo::analyze() {
  println("ANALYZING ", *this);

  gather_redefinitions(this->allocation->getCallInst(), this->redefinitions);

  for (const auto &[func, redefs] : this->redefinitions) {
    for (auto *redef : redefs) {
      gather_uses_to_proxy(*redef,
                           this->offsets,
                           this->to_encode,
                           this->to_decode,
                           this->to_addkey);
    }
  }

#if 0
  for (const auto &[func, redefs] : this->redefinitions) {
    println("IN ", func->getName());
    for (auto *redef : redefs) {
      println("  REDEF ", *redef);
    }
  }
#endif
}

Type &ObjectInfo::get_type() const {
  auto *type = &this->allocation->getType();
  for (auto offset : this->offsets) {
    if (auto *struct_type = dyn_cast<StructType>(type)) {
      type = &struct_type->getFieldType(offset);
    } else if (auto *collection_type = dyn_cast<CollectionType>(type)) {
      type = &collection_type->getElementType();
    }
  }

  return *type;
}

llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const ObjectInfo &info) {
  os << "(" << *info.allocation << ")";
  for (auto offset : info.offsets) {
    if (offset == unsigned(-1)) {
      os << "[*]";
    } else {
      os << "." << std::to_string(offset);
    }
  }

  return os;
}

ProxyInsertion::ProxyInsertion(llvm::Module &M) : M(M) {

  // Register the bit{map,set} implementations.
  Implementation::define({
      Implementation( // bitset<T>
          "bitset",
          AssocType::get(TypeVariable::get(), VoidType::get())),

      Implementation( // bitmap<T, U>
          "bitmap",
          AssocType::get(TypeVariable::get(), TypeVariable::get())),

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
  println("Verified module post-proxy insertion.");

  reify_tempargs(M);

  for (auto &F : M) {
    if (not F.empty()) {
      println("Verifying ", F.getName(), "...");
      if (llvm::verifyFunction(F, &llvm::errs())) {
        println(F);
        MEMOIR_UNREACHABLE("Failed to verify ", F.getName());
      }
    }
  }
  println("Verified module post-temparg reify.");
}

void ProxyInsertion::gather_assoc_objects(
    vector<ObjectInfo> &allocations,
    AllocInst &alloc,
    Type &type,
    vector<unsigned> offsets,
    std::optional<SelectionMetadata> selection,
    unsigned selection_index) {

  if (not selection) {
    selection = Metadata::get<SelectionMetadata>(alloc);
  }

  if (auto *struct_type = dyn_cast<StructType>(&type)) {
    for (unsigned field = 0; field < struct_type->getNumFields(); ++field) {

      auto new_offsets = offsets;
      new_offsets.push_back(field);

      this->gather_assoc_objects(
          allocations,
          alloc,
          struct_type->getFieldType(field),
          new_offsets,
          Metadata::get<SelectionMetadata>(*struct_type, field));
    }

  } else if (auto *collection_type = dyn_cast<CollectionType>(&type)) {
    auto &elem_type = collection_type->getElementType();

    // If this is an assoc, add the object information.
    if (isa<AssocType>(collection_type)) {
      // TODO: ensure that this nested collection has no selection.
      if (not selection or not selection->getImplementation(offsets.size())) {
        allocations.push_back(ObjectInfo(alloc, offsets));
      } else {
        println("FOUND EXISTING SELECTION FOR ", alloc);
      }
    }

    // Recurse on the element.
    auto new_offsets = offsets;
    new_offsets.push_back(-1);

    this->gather_assoc_objects(allocations,
                               alloc,
                               elem_type,
                               new_offsets,
                               selection,
                               ++selection_index);
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

  println("Found ", this->objects.size(), " objects.");

  // Gather statistics about each of the objects.
  for (auto &info : this->objects) {
    info.analyze();
  }

  // Use a heuristic to group together objects.
  set<const ObjectInfo *> used = {};
  for (auto it = this->objects.begin(); it != this->objects.end(); ++it) {
    auto &info = *it;

    if (used.count(&info) > 0) {
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

    // Find all other allocations that share a basic block with this one.
    for (auto it2 = std::next(it); it2 != this->objects.end(); ++it2) {
      auto &other = *it2;

      if (used.count(&other) > 0) {
        continue;
      }

      // Check that the key types match.
      auto *other_alloc = other.allocation;
      auto &other_type = MEMOIR_SANITIZE(dyn_cast<AssocType>(&other.get_type()),
                                         "Non-assoc type, unhandled.");

      if (&type.getKeyType() != &other_type.getKeyType()) {
        continue;
      }

      // Check that they share a parent basic block.
      // NOTE: this is overly conservative
      auto *other_func = other.allocation->getFunction();
      if (func != other_func) {
        continue;
      }

      candidate.push_back(&other);
    }

    // Compute the benefit of each object in the candidate.
    uint32_t candidate_benefit = 0;
    set<const ObjectInfo *> has_benefit = {};
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

namespace detail {

llvm::FunctionCallee create_addkey_function(llvm::Module &M,
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

  println("Creating addkey.");
  println("  MEMOIR key type ", key_type);
  println("    LLVM key type ", *llvm_key_type);

  // Create the addkey functions for this proxy.
  vector<llvm::Type *> addkey_params = { llvm_key_type };
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
                            set<llvm::Function *> &functions) {

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
                            set<llvm::Function *> &functions) {
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

void add_tempargs(map<llvm::Function *, llvm::Instruction *> &local_patches,
                  llvm::ArrayRef<set<llvm::Use *>> uses_to_patch,
                  llvm::Instruction &patch_with,
                  Type &patch_type) {

  MemOIRBuilder builder(&patch_with);
  auto &context = builder.getContext();
  auto &module = builder.getModule();

  // Unpack the patch.
  auto *patch_func = patch_with.getFunction();
  auto *llvm_patch_type = patch_type.get_llvm_type(context);

  // Track the local patch for each function.
  local_patches[patch_func] = &patch_with;

  // Find the set of functions that need the patch.
  set<llvm::Function *> functions = { patch_func };
  for (auto &uses : uses_to_patch) {
    for (auto *use : uses) {
      if (auto *inst = dyn_cast<llvm::Instruction>(use->getUser())) {
        functions.insert(inst->getFunction());
      }
    }
  }

  // Close the set of functions.
  set<llvm::Function *> forward = {};
  set<llvm::Function *> backward = {};
  for (auto *func : functions) {
    collect_callers(*func, forward);
    collect_callees(*func, backward);
  }

  for (auto *func : forward) {
    println("CALLER ", func->getName());
    if (backward.count(func) > 0) {
      println("  CALLEE ", func->getName());
      functions.insert(func);
    }
  }

  // Determine the set of functions that we need to pass the proxy to.
  map<llvm::CallBase *, llvm::GlobalVariable *> calls_to_patch = {};
  map<FoldInst *, llvm::GlobalVariable *> folds_to_patch = {};
  for (auto *func : functions) {

    println("ADDING TEMPARGS ", func->getName());

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
          println("  USER ", *fold);
          folds_to_patch[fold] = global;
        }
      } else if (not into<MemOIRInst>(call)) {
        if (call->getCalledFunction() == func) {
          println("  USER ", *call);
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
void update_candidates(std::forward_iterator auto candidates_begin,
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

static void find_fold_users(llvm::Value &V,
                            llvm::ArrayRef<unsigned> offsets,
                            vector<FoldInst *> &folds) {
  println("  REDEF ", V);
  for (auto &use : V.uses()) {
    if (auto *fold = into<FoldInst>(use.getUser())) {
      if (use == fold->getObjectAsUse()) {
        auto distance = detail::indices_match_offsets(*fold, offsets);
        if (distance == -1) {
          // Mismatch.
        } else if (distance == int32_t(offsets.size())) {
          auto found = std::find(folds.begin(), folds.end(), fold);
          if (found == folds.end()) {
            println("    TO MUTATE ", *fold);
            folds.push_back(fold);
          }
        } else if (distance < int32_t(offsets.size())) {
          if (auto *elem_arg = fold->getElementArgument()) {
            find_fold_users(*elem_arg,
                            llvm::ArrayRef<unsigned>(
                                std::next(offsets.begin(), distance + 1),
                                offsets.end()),
                            folds);
          }
          // TODO: this check is not enough for everything that could happen, we
          // need to move this recursion case into the redefinitions computation
        }
      }
    }
  }

  return;
}

} // namespace detail

bool ProxyInsertion::transform() {

  bool modified = false;

  // Collect the set of redefinitions for each allocation involved.
  for (auto candidates_it = this->candidates.begin();
       candidates_it != this->candidates.end();
       ++candidates_it) {

    modified |= true;

    auto &candidate = *candidates_it;

    println();
    println("PROXYING CANDIDATE:");
    for (const auto *candidate_info : candidate) {
      println("  ", *candidate_info);
    }

    // Unpack the first object information.
    auto *first_info = candidate.front();
    auto *alloc = first_info->allocation;
    auto *type = &alloc->getType();

    // Get the nested object type.
    for (auto offset : first_info->offsets) {
      if (auto *struct_type = dyn_cast<StructType>(type)) {
        type = &struct_type->getFieldType(offset);
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

    // Union together all of the uses that need to be encoded, decoded or
    // added into the proxy space.
    set<llvm::Use *> to_encode = {};
    set<llvm::Use *> to_decode = {};
    set<llvm::Use *> to_addkey = {};
    for (auto *info : candidate) {
      for (const auto &[func, uses] : info->to_encode) {
        to_encode.insert(uses.begin(), uses.end());
      }

      for (const auto &[func, uses] : info->to_decode) {
        to_decode.insert(uses.begin(), uses.end());
      }

      for (const auto &[func, uses] : info->to_addkey) {
        to_addkey.insert(uses.begin(), uses.end());
      }
    }

    println();
    println("  before trimming:");
    println("  ", to_encode.size(), " uses to encode");
    println("  ", to_decode.size(), " uses to decode");
    println("  ", to_addkey.size(), " uses to addkey");

    // Trim uses that dont need to be decoded because they are only used to
    // compare against other values that need to be decoded.
    set<llvm::Use *> trim_to_decode = {};
    for (auto *use : to_decode) {
      auto *user = use->getUser();

      for (auto *other_use : to_decode) {
        if (use == other_use) {
          continue;
        }

        auto *other_user = other_use->getUser();
        if (user != other_user) {
          continue;
        }

        if (auto *cmp = dyn_cast<llvm::CmpInst>(user)) {
          if (cmp->isEquality()) {
            trim_to_decode.insert(use);
            trim_to_decode.insert(other_use);
          }
        }
      }
    }

    // Trim uses that dont need to be encoded because they are produced by a
    // use that needs decoded.
    set<llvm::Use *> trim_to_encode = {};
    for (auto uses : { to_encode, to_addkey }) {
      for (auto *use : uses) {
        if (to_decode.count(use) > 0) {
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

    {
      println("  trimmed:");
      println("  ", trim_to_encode.size(), " uses to encode");
      for (auto *use : trim_to_encode) {
        infoln("    ", *use->get(), " in ", *use->getUser());
      }

      println("  ", trim_to_decode.size(), " uses to decode");
      for (auto *use : trim_to_decode) {
        infoln("    ", *use->get(), " in ", *use->getUser());
      }
    }

    {
      println("  after trimming:");
      println("  ", to_encode.size(), " uses to encode");
      for (auto *use : to_encode) {
        infoln("    ", *use->get(), " in ", *use->getUser());
      }

      println("  ", to_decode.size(), " uses to decode");
      for (auto *use : to_decode) {
        infoln("    ", *use->get(), " in ", *use->getUser());
      }

      println("  ", to_addkey.size(), " uses to addkey");
      for (auto *use : to_addkey) {
        infoln("    ", *use->get(), " in ", *use->getUser());
      }
    }

    // TODO: Find a point that dominates all of the object allocations.

    // Find the construction point for the encoder and decoder.
    auto *construction_point = &alloc->getCallInst();

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
    map<llvm::Function *, llvm::Instruction *> function_to_encoder = {};
    if (build_encoder) {
      detail::add_tempargs(function_to_encoder,
                           { to_encode, to_addkey },
                           *encoder,
                           *encoder_type);
    }

    map<llvm::Function *, llvm::Instruction *> function_to_decoder = {};
    if (build_decoder) {
      detail::add_tempargs(function_to_decoder,
                           { to_decode, to_addkey },
                           *decoder,
                           *decoder_type);
    }

    // Create the addkey function.
    auto addkey_callee = detail::create_addkey_function(this->M,
                                                        key_type,
                                                        build_encoder,
                                                        encoder_type,
                                                        build_decoder,
                                                        decoder_type);

    // For each of the uses to encode, encode them.
    for (auto *use : to_encode) {

      // Unpack the use.
      auto *used = use->get();

      // Find the use's program point.
      auto *user_as_inst = dyn_cast<llvm::Instruction>(use->getUser());
      auto *user_bb = user_as_inst->getParent();
      auto *user_func = user_bb->getParent();

      // TODO: Extend this to re-use en/decodings that dominate the given
      // program point.

      // Fetch the encoder for this function.
      auto *encoder = function_to_encoder.at(user_func);

      // Compute the insertion point.
      llvm::Instruction *program_point = nullptr;
      if (user_as_inst) {
        if (auto *phi = dyn_cast<llvm::PHINode>(user_as_inst)) {
          MEMOIR_UNREACHABLE("En/decoding uses by PHI is unhandled.");
        } else {
          program_point = user_as_inst;
        }

      } else {
        MEMOIR_UNREACHABLE("Failed to find a point to encode the value!");
      }

      builder.SetInsertPoint(program_point);

      // Handle has operations separately.
      if (auto *has = into<HasInst>(user_as_inst)) {
        if (detail::is_last_index(use, has->index_operands_end())) {
          // Construct an if-else block.
          // if (has(encoder, key))
          //   i = read(encoder, key)
          //   h = has(collection, i)
          // h' = PHI(h, false)
          auto *cond = &builder.CreateHasInst(encoder, used)->getCallInst();
          auto *phi = builder.CreatePHI(cond->getType(), 2);

          auto *then_terminator =
              llvm::SplitBlockAndInsertIfThen(cond,
                                              phi,
                                              /* unreachable? */ false);

          user_as_inst->moveBefore(then_terminator);

          builder.SetInsertPoint(user_as_inst);

          auto *encoded =
              &builder.CreateReadInst(size_type, encoder, used)->getCallInst();

          use->set(encoded);

          user_as_inst->replaceAllUsesWith(phi);

          if (auto *call = dyn_cast<llvm::CallBase>(user_as_inst)) {
            call->removeParamAttr(use->getOperandNo(),
                                  llvm::Attribute::AttrKind::NonNull);
          }

          auto *then_bb = then_terminator->getParent();
          phi->addIncoming(user_as_inst, then_bb);

          auto *else_bb = cond->getParent();
          auto *false_constant = llvm::ConstantInt::getFalse(context);
          phi->addIncoming(false_constant, else_bb);

          continue;
        }
      }

      // In the common case, read the encoded value and update the use with
      // it.
      auto *encoded =
          &builder.CreateReadInst(size_type, encoder, used)->getCallInst();

      use->set(encoded);

      if (auto *call = dyn_cast<llvm::CallBase>(user_as_inst)) {
        call->removeParamAttr(use->getOperandNo(),
                              llvm::Attribute::AttrKind::NonNull);
      }
    }

    // For each of the uses to encode, encode them.
    for (auto *use : to_addkey) {

      // Unpack the use.
      auto *used = use->get();

      // Find the use's program point.
      auto *user_as_inst = dyn_cast<llvm::Instruction>(use->getUser());
      auto *user_bb = user_as_inst->getParent();
      auto *user_func = user_bb->getParent();

      // TODO: Extend this to re-use en/decodings that dominate the given
      // program point.

      // Compute the insertion point.
      llvm::Instruction *program_point = nullptr;
      if (user_as_inst) {
        if (auto *phi = dyn_cast<llvm::PHINode>(user_as_inst)) {
          MEMOIR_UNREACHABLE("En/decoding uses by PHI is unhandled.");
        } else {
          program_point = user_as_inst;
        }

      } else {
        MEMOIR_UNREACHABLE("Failed to find a point to encode the value!");
      }

      builder.SetInsertPoint(program_point);

      vector<llvm::Value *> args = { use->get() };
      if (build_encoder) {
        auto *encoder = function_to_encoder.at(user_func);
        args.push_back(encoder);
      }
      if (build_decoder) {
        auto *decoder = function_to_decoder.at(user_func);
        args.push_back(decoder);
      }

      auto *encoded = builder.CreateCall(addkey_callee, args);

      use->set(encoded);

      if (auto *call = dyn_cast<llvm::CallBase>(user_as_inst)) {
        call->removeParamAttr(use->getOperandNo(),
                              llvm::Attribute::AttrKind::NonNull);
      }
    }

    for (auto *use : to_decode) {

      // Unpack the use.
      auto *used = use->get();

      // Find the use's program point.
      auto *user_as_inst = dyn_cast<llvm::Instruction>(use->getUser());
      auto *user_bb = user_as_inst->getParent();
      auto *user_func = user_bb->getParent();

      // TODO: Extend this to re-use en/decodings that dominate the given
      // program point.

      // Fetch the decoder for this function.
      auto *decoder = function_to_decoder.at(user_func);

      // Compute the insertion point.
      llvm::Instruction *program_point = nullptr;
      if (user_as_inst) {
        if (auto *phi = dyn_cast<llvm::PHINode>(user_as_inst)) {
          auto *incoming_block = phi->getIncomingBlock(*use);
          // TODO: this may be unsound if the terminator is conditional.
          program_point = incoming_block->getTerminator();
        } else {
          program_point = user_as_inst;
        }

      } else {
        MEMOIR_UNREACHABLE("Failed to find a point to decode the value!");
      }

      builder.SetInsertPoint(program_point);

      // Then, read the value from the decoder.
      auto *decoded =
          &builder.CreateReadInst(key_type, decoder, { use->get() })
               ->getCallInst();

      println("DECODED ", *decoded);
      println("  DEBUG ", *program_point);

      use->set(decoded);
    }

    // Set the selection of the collection.
    map<ObjectInfo *, vector<FoldInst *>> folds_to_mutate = {};
    for (auto *info : candidate) {

      println("Updating selection and types");
      println("  ", *info);

      // Determine _where_ to attach the selection.
      auto selection =
          Metadata::get_or_add<SelectionMetadata>(*info->allocation);
      unsigned selection_index = 0;
      auto *type = &info->allocation->getType();
      for (auto offset : info->offsets) {
        if (auto *struct_type = dyn_cast<StructType>(type)) {
          auto &field_type = struct_type->getFieldType(offset);
          type = &field_type;

          selection =
              Metadata::get_or_add<SelectionMetadata>(*struct_type, offset);
          selection_index = 0;

        } else if (auto *collection_type = dyn_cast<CollectionType>(type)) {
          auto &elem_type = collection_type->getElementType();
          type = &elem_type;

          ++selection_index;
        }
      }

      println("  TYPE ", *type);

      // Unpack the type information.
      if (auto *assoc_type = dyn_cast<AssocType>(type)) {

        // Set the implementation.
        if (isa<VoidType>(&assoc_type->getValueType())) {
          selection.setImplementation("bitset", selection_index);
        } else {
          selection.setImplementation("bitmap", selection_index);
        }

        // Update the type of the key in the fold bodies.
        // TODO: collect folds ahead of time for all info in the candidate, it
        // is getting invalidated right now.
        auto &to_mutate = folds_to_mutate[info];
        for (auto [func, redefs] : info->redefinitions) {
          for (auto *redef : redefs) {
            detail::find_fold_users(*redef, info->offsets, to_mutate);
          }
        }

      } else {
        MEMOIR_UNREACHABLE("Proxying non-assoc types is not yet supported.");
      }
    }

    // Mutate types in the program.
    for (auto *info : candidate) {

      // For each of the folds, create a new function with the updated type.
      set<llvm::Function *> functions_to_delete = {};
      auto &to_mutate = folds_to_mutate[info];
      for (auto *fold : to_mutate) {

        println("MUTATE ", *fold);

        // Fetch the index argument.
        auto &index_arg = fold->getIndexArgument();

        // Update the type of the function to match.
        auto *function = index_arg.getParent();
        auto *function_type = function->getFunctionType();

        bool found_real_use = false;
        if (function->hasNUsesOrMore(2)) {
          for (auto &use : function->uses()) {
            auto *user = use.getUser();
            if (not into<RetPHIInst>(*user)) {
              println("  REAL ", *use.getUser());
              MEMOIR_ASSERT(not found_real_use,
                            "Fold body has more than one use!");
              found_real_use = true;
            }
          }
        }

        // Rebuild the function type.
        vector<llvm::Type *> params(function_type->param_begin(),
                                    function_type->param_end());
        params[index_arg.getArgNo()] = &llvm_size_type;

        auto *new_function_type =
            llvm::FunctionType::get(function_type->getReturnType(),
                                    params,
                                    function_type->isVarArg());

        // Create a new function with the new function type.
        auto *new_function = llvm::Function::Create(new_function_type,
                                                    function->getLinkage(),
                                                    function->getName(),
                                                    this->M);

        // Clone the function with a changed parameter type.
        llvm::ValueToValueMapTy vmap;
        for (auto &old_arg : function->args()) {
          auto *new_arg = new_function->getArg(old_arg.getArgNo());
          vmap.insert({ &old_arg, new_arg });
        }
        llvm::SmallVector<llvm::ReturnInst *, 8> returns;
        llvm::CloneFunctionInto(new_function,
                                function,
                                vmap,
                                llvm::CloneFunctionChangeType::LocalChangesOnly,
                                returns);

        // Remove any pointer related attributes from the argument.
        auto attr_list = new_function->getAttributes();
        new_function->setAttributes(
            attr_list.removeParamAttributes(context, index_arg.getArgNo()));

        // Use the vmap to update any of the folds that we are patching.
        for (auto &[_, other_to_mutate] : folds_to_mutate) {
          for (auto &other_fold : other_to_mutate) {
            // Skip the fold we just updated.
            if (other_fold == fold) {
              continue;
            }

            // If the parent function is the same as the old function, find this
            // fold in the vmap.
            auto *parent_function = other_fold->getFunction();
            if (parent_function == function) {
              auto *new_inst = &*vmap[&other_fold->getCallInst()];
              auto *new_fold = into<FoldInst>(new_inst);

              // Update in-place.
              other_fold = new_fold;
            }
          }
        }

        // Remap any of the candidates that have been cloned.
        detail::update_candidates(std::next(candidates_it),
                                  this->candidates.end(),
                                  *function,
                                  *new_function,
                                  vmap);

        // Update the body of this fold.
        auto &body_use = fold->getBodyOperandAsUse();
        body_use.set(new_function);

        function->replaceAllUsesWith(new_function);

        new_function->takeName(function);

        function->deleteBody();

        functions_to_delete.insert(function);
      }

      for (auto *func : functions_to_delete) {
        func->eraseFromParent();
      }
    }
  }

  return modified;
}

} // namespace folio
