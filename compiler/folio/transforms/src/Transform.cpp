#include <numeric>

#include "llvm/IR/AttributeMask.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "memoir/ir/Builder.hpp"
#include "memoir/ir/Instructions.hpp"
#include "memoir/support/AssocList.hpp"
#include "memoir/support/WorkList.hpp"
#include "memoir/transforms/utilities/MutateType.hpp"
#include "memoir/utility/Metadata.hpp"

#include "folio/transforms/ProxyInsertion.hpp"
#include "folio/transforms/Utilities.hpp"

using namespace llvm::memoir;

namespace folio {

// Command line options.
static llvm::cl::opt<bool> disable_total_proxy(
    "disable-total-proxy",
    llvm::cl::desc("Disable total proxy optimization"),
    llvm::cl::init(false));

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
    llvm::ArrayRef<llvm::memoir::Vector<CoalescedUses>> uses_to_patch,
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
  for (auto &coalesced : uses_to_patch) {
    for (auto &uses : coalesced) {
      for (auto *use : uses) {
        if (auto *inst = dyn_cast<llvm::Instruction>(use->getUser())) {
          functions.insert(inst->getFunction());
        }
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
    auto *global = &create_global_ptr(module, "temparg");

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
    for (const auto &[base, redefs] : base_to_redefs.second) {
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

      auto selection =
          ProxyInsertion::get_enumerated_impl(*assoc_type, is_nested);

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

bool ProxyInsertion::transform() {

  bool modified = false;

  // Optimize the uses in each candidate.
  for (auto &candidate : this->candidates) {
    infoln("OPTIMIZING ", candidate);
    candidate.optimize(this->get_dominator_tree, this->get_bounds_checks);
  }

  // Prepare the program for transformation.
  this->prepare();

  println(this->M);

  // Transform the program for each candidate.
  for (auto candidates_it = this->candidates.begin();
       candidates_it != this->candidates.end();
       ++candidates_it) {
    modified |= true;

    auto &candidate = *candidates_it;

    // Unpack the candidate uses.
    auto &encoded = candidate.encoded;
    auto &decoded = candidate.decoded;
    auto &added = candidate.added;

    infoln("PROXYING ", candidate);

    // Find the construction point for the candidate.
    auto &domtree = this->get_dominator_tree(candidate.function());
    auto &construction_point = candidate.construction_point(domtree);

    // Fetch LLVM context.
    auto &context = construction_point.getContext();
    auto &module = MEMOIR_SANITIZE(construction_point.getModule(),
                                   "Construction point has no module.");
    auto &data_layout = module.getDataLayout();

    // Allocate the proxy.
    MemOIRBuilder builder(&construction_point);

    auto &key_type = candidate.key_type();
    auto &size_type = Type::get_size_type(data_layout);
    auto &llvm_size_type = *size_type.get_llvm_type(context);
    auto size_type_bitwidth = size_type.getBitWidth();

    // Determine which proxies we need.
    bool build_encoder = encoded.size() > 0 or added.size() > 0;
    bool build_decoder = decoded.size() > 0;

    // Allocate the encoder.
    llvm::Instruction *encoder = nullptr;
    Type *encoder_type = nullptr;
    if (build_encoder) {
      encoder_type = &AssocType::get(key_type, size_type);
      auto *encoder_alloc =
          builder.CreateAllocInst(*encoder_type, {}, "proxy.encode.");
      encoder = &encoder_alloc->getCallInst();
    }

    // Allocate the decoder.
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
                   { encoded, added },
                   *encoder,
                   *encoder_type,
                   "encoder.");
    }

    Map<llvm::Function *, llvm::Instruction *> function_to_decoder = {
      { NULL, decoder }
    };
    if (build_decoder) {
      add_tempargs(function_to_decoder,
                   { decoded, added },
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
    for (const auto &[func, values] : candidate.encoded_values) {
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

} // namespace folio
