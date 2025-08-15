#include <numeric>

#include "llvm/IR/AttributeMask.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "memoir/ir/Builder.hpp"
#include "memoir/ir/Instructions.hpp"
#include "memoir/raising/RepairSSA.hpp"
#include "memoir/support/AssocList.hpp"
#include "memoir/support/SortedVector.hpp"
#include "memoir/support/WorkList.hpp"
#include "memoir/transforms/utilities/MutateType.hpp"
#include "memoir/transforms/utilities/PromoteGlobals.hpp"
#include "memoir/utility/Metadata.hpp"

#include "folio/ProxyInsertion.hpp"
#include "folio/Utilities.hpp"

using namespace llvm::memoir;

namespace folio {

// Command line options.
static llvm::cl::opt<bool> disable_total_proxy(
    "disable-total-proxy",
    llvm::cl::desc("Disable total proxy optimization"),
    llvm::cl::init(true));

llvm::Instruction &Candidate::construction_point(
    llvm::DominatorTree &domtree) const {
  // Find the construction point for the encoder and decoder.
  llvm::Instruction *construction_point =
      dyn_cast<llvm::Instruction>(&this->front()->value());

  // Find a point that dominates all of the object allocations.
  for (const auto *other : *this) {
    if (auto *inst = dyn_cast<llvm::Instruction>(&other->value()))
      construction_point =
          domtree.findNearestCommonDominator(construction_point, inst);
  }

  return MEMOIR_SANITIZE(
      construction_point,
      "Failed to find a construction point for the candidate!");
}

static llvm::Function &create_addkey_function(llvm::Module &M,
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

  // Unpack the function arguments.
  auto arg_idx = 0;
  auto *key = addkey_function.getArg(arg_idx++);

  llvm::Value *encoder_inout = NULL;
  if (build_encoder) {
    encoder_inout = addkey_function.getArg(arg_idx++);
  }

  llvm::Value *decoder_inout = NULL;
  if (build_decoder) {
    decoder_inout = addkey_function.getArg(arg_idx++);
  }

  auto *ret_bb = llvm::BasicBlock::Create(context, "", &addkey_function);

  MemOIRBuilder builder(ret_bb);

  llvm::Value *encoder = NULL, *decoder = NULL;
  if (build_encoder) {
    encoder = builder.CreateLoad(llvm_ptr_type, encoder_inout, "enc.");
    builder.CreateAssertTypeInst(encoder, *encoder_type);
  }
  if (build_decoder) {
    decoder = builder.CreateLoad(llvm_ptr_type, decoder_inout, "dec.");
    builder.CreateAssertTypeInst(decoder, *decoder_type);
  }

  // Check if the encoder has the given key.
  auto *has_key = &builder.CreateHasInst(encoder, key)->getCallInst();

  // Construct the join block.
  auto *phi = builder.CreatePHI(llvm_size_type, 2);
  builder.CreateRet(phi);

  // Split the block and insert an if-then-else conditioned on the has-key.
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
  if (build_encoder) {
    new_index = &builder.CreateSizeInst(encoder)->getCallInst();
    auto *insert = builder.CreateInsertInst(encoder, { key });
    auto *write = builder.CreateWriteInst(size_type,
                                          new_index,
                                          &insert->asValue(),
                                          { key });
    builder.CreateStore(&write->asValue(), encoder_inout);
  }

  if (build_decoder) {
    if (not new_index) {
      new_index = &builder.CreateSizeInst(decoder)->getCallInst();
    }
    auto *end = &builder.CreateEndInst()->getCallInst();
    auto *insert = builder.CreateInsertInst(decoder, { end });
    auto *write = builder.CreateWriteInst(key_type,
                                          key,
                                          &insert->asValue(),
                                          { new_index });
    builder.CreateStore(&write->asValue(), decoder_inout);
  }

  // Update the PHIs
  MEMOIR_ASSERT(read_index, "Read index is NULL");
  phi->addIncoming(read_index, then_bb);
  MEMOIR_ASSERT(new_index, "New index is NULL");
  phi->addIncoming(new_index, else_bb);

  return addkey_function;
}

llvm::FunctionCallee Candidate::addkey_callee() {
  if (this->addkey_function == NULL) {
    this->addkey_function = &create_addkey_function(this->module(),
                                                    this->key_type(),
                                                    this->build_encoder(),
                                                    &this->encoder_type(),
                                                    this->build_decoder(),
                                                    &this->decoder_type());
  }

  return llvm::FunctionCallee(this->addkey_function);
}

static Pair<llvm::Value *, llvm::Type *> get_ptr(Mapping &mapping,
                                                 llvm::Value &value,
                                                 ObjectInfo *base) {
  if (parent<llvm::Function>(value) == base->function()) {
    auto &local = mapping.local(*base);
    return make_pair(&local, local.getAllocatedType());
  }

  auto &global = mapping.global(*base);
  return make_pair(&global, global.getValueType());
}

static llvm::Value *fetch_mapping(Mapping &mapping,
                                  Type &mapping_type,
                                  MemOIRBuilder &builder,
                                  llvm::Value &value,
                                  ObjectInfo *base,
                                  const llvm::Twine &name = "") {
  auto [ptr, type] = get_ptr(mapping, value, base);
  auto *val = builder.CreateLoad(type, ptr, name);
  builder.CreateAssertTypeInst(val, mapping_type, name.concat(".type"));

  return val;
}

static llvm::Value *fetch_encoder(Candidate &candidate,
                                  MemOIRBuilder &builder,
                                  llvm::Value &value,
                                  ObjectInfo *base) {
  return fetch_mapping(candidate.encoder,
                       candidate.encoder_type(),
                       builder,
                       value,
                       base,
                       "enc");
}

static llvm::Value *fetch_decoder(Candidate &candidate,
                                  MemOIRBuilder &builder,
                                  llvm::Value &value,
                                  ObjectInfo *base) {
  return fetch_mapping(candidate.decoder,
                       candidate.decoder_type(),
                       builder,
                       value,
                       base,
                       "dec");
}

#ifndef PRINT_VALUES
#  define PRINT_VALUES 0
#endif

llvm::Instruction &Candidate::has_value(MemOIRBuilder &builder,
                                        llvm::Value &value,
                                        ObjectInfo *base) {
#if PRINT_VALUES
  builder.CreateErrorf("HAS %u\n", { &value });
#endif

  auto *enc = fetch_encoder(*this, builder, value, base);
  return builder.CreateHasInst(enc, { &value })->getCallInst();
};

llvm::Value &Candidate::decode_value(MemOIRBuilder &builder,
                                     llvm::Value &value,
                                     ObjectInfo *base) {
#if PRINT_VALUES
  builder.CreateErrorf("DEC %lu ", { &value });
#endif

  auto *dec = fetch_decoder(*this, builder, value, base);
  auto &decoded =
      builder.CreateReadInst(this->key_type(), dec, { &value })->asValue();

#if PRINT_VALUES
  builder.CreateErrorf("= %u\n", { &decoded });
#endif

  return decoded;
};

llvm::Value &Candidate::encode_value(MemOIRBuilder &builder,
                                     llvm::Value &value,
                                     ObjectInfo *base) {
#if PRINT_VALUES
  builder.CreateErrorf("ENC %u ", { &value });
#endif

  auto *enc = fetch_encoder(*this, builder, value, base);

  auto &size_type = Type::get_size_type(this->module().getDataLayout());
  auto &encoded = builder.CreateReadInst(size_type, enc, &value)->asValue();

#if PRINT_VALUES
  builder.CreateErrorf("= %lu\n", { &encoded });
#endif

  return encoded;
};

llvm::Value &Candidate::add_value(MemOIRBuilder &builder,
                                  llvm::Value &value,
                                  ObjectInfo *base) {
#if PRINT_VALUES
  builder.CreateErrorf("ADD %u ", { &value });
#endif

  Vector<llvm::Value *> args = { &value };

  llvm::AllocaInst *enc_inout = NULL, *dec_inout = NULL;

  if (this->build_encoder()) {
    // Fetch the current state of the encoder.
    auto *enc = fetch_encoder(*this, builder, value, base);

    // Create an inout stack variable and initialize it.
    enc_inout = builder.CreateLocal(builder.getPtrTy());
    builder.CreateLifetimeStart(enc_inout);
    builder.CreateStore(enc, enc_inout);

    // Pass the inout stack variable to the function.
    args.push_back(enc_inout);
  }
  if (this->build_decoder()) {
    // Fetch the current state of the decoder.
    auto *dec = fetch_decoder(*this, builder, value, base);

    // Create an inout stack variable and initialize it.
    dec_inout = builder.CreateLocal(builder.getPtrTy());
    builder.CreateLifetimeStart(dec_inout);
    builder.CreateStore(dec, dec_inout);

    // Pass the inout stack variable to the function.
    args.push_back(dec_inout);
  }

  // Call the addkey function.
  auto &encoded =
      MEMOIR_SANITIZE(builder.CreateCall(this->addkey_callee(), args),
                      "Failed to create call to addkey!");

  // Load the inout results.
  if (enc_inout) {
    auto *enc = builder.CreateLoad(builder.getPtrTy(), enc_inout);
    builder.CreateLifetimeEnd(enc_inout);
    auto [enc_state, enc_type] = get_ptr(this->encoder, value, base);
    builder.CreateStore(enc, enc_state);
  }
  if (dec_inout) {
    auto *dec = builder.CreateLoad(builder.getPtrTy(), dec_inout);
    builder.CreateLifetimeEnd(dec_inout);
    auto [dec_state, dec_type] = get_ptr(this->decoder, value, base);
    builder.CreateStore(dec, dec_state);
  }

#if PRINT_VALUES
  builder.CreateErrorf("= %lu\n", { &encoded });
#endif

  return encoded;
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
  for (auto it = candidates_begin; it != candidates_end; ++it)
    it->update(old_func, new_func, vmap, /* delete old? */ true);
}

static void promote_locals(Mapping &mapping,
                           ProxyInsertion::GetDominatorTree get_domtree) {

  // FIXME: Something is going wrong when calculating the dominance frontier.
  return;

  // Collect the stack variables to promote.
  Map<llvm::Function *, Vector<llvm::AllocaInst *>> locals = {};
  for (const auto &[base, var] : mapping.locals()) {
    auto *func = var->getFunction();
    MEMOIR_ASSERT(func, "Stack variable has no parent function!");
    if (var) {
      locals[func].push_back(var);
    }
  }

  for (const auto &[func, vars] : locals) {
    // Fetch the dominator tree.
    auto &domtree = get_domtree(*func);
    repair_ssa(vars, domtree);
  }
}

static void promote(Candidate &candidate,
                    ProxyInsertion::GetDominatorTree get_domtree) {
  // Promote local variabels to registers.
  promote_locals(candidate.encoder, get_domtree);
  promote_locals(candidate.decoder, get_domtree);

  // Collect all of the globals that can be promoted.
  Vector<llvm::GlobalVariable *> globals_to_promote;
  for (const auto &mapping : { candidate.encoder, candidate.decoder }) {
    for (const auto &[base, global] : mapping.globals()) {
      if (global and global_is_promotable(*global)) {
        globals_to_promote.push_back(global);
      }
    }
  }

  // Promote the globals.
  promote_globals(globals_to_promote);
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

bool is_total_proxy(ObjectInfo &obj, const Vector<CoalescedUses> &added) {

  println();
  println("TOTAL PROXY? ", obj);

  // Check that the object is not a propagator.
  // NOTE: this is conservative, but the check for a propagator to be a total
  // proxt is difficult.
  if (obj.is_propagator()) {
    println("NO, propagator");
    return false;
  }

  // Check that there is guaranteed to be one of these objects for each
  // allocation.
  for (auto offset : obj.offsets()) {
    if (offset == unsigned(-1)) {
      println("NO, not singular");
      return false;
    }
  }

  // Check that this value is never cleared or removed from.
  bool monotonic = true;
  for (const auto &[func, info] : obj.info())
    for (const auto &redef : info.redefinitions) {
      auto *value = &redef.value();
      if (into<RemoveInst>(value) or into<ClearInst>(value)) {
        println("NO, keys are removed");
        return false;
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
        if (not obj.is_redefinition(access->getObject())) {
          continue;
        }

        // Check that this access is at the correct offset.
        auto distance = access->match_offsets(obj.offsets());
        if (not distance) {
          continue;
        }

        if (distance.value() < obj.offsets().size()) {
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

static void decode_candidate_uses(Candidate &candidate,
                                  Set<llvm::Use *> &handled) {

  for (const auto &[base, func_uses] : candidate.to_decode)
    for (const auto &[func, uses] : func_uses)
      for (auto *use : uses) {
        if (handled.contains(use)) {
          continue;
        } else {
          handled.insert(use);
        }

        auto *used = use->get();
        auto *user = dyn_cast<llvm::Instruction>(use->getUser());

        // Compute the insertion point.
        MemOIRBuilder builder(insertion_point(*use));

        auto &decoded = candidate.decode_value(builder, *used, base);

        use->set(&decoded);
      }
}

static void decode_uses(Vector<Candidate> &candidates) {
  Set<llvm::Use *> handled = {};
  for (auto &candidate : candidates)
    decode_candidate_uses(candidate, handled);
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

static llvm::Value &encode_use(Candidate &candidate,
                               MemOIRBuilder &builder,
                               llvm::Use &use,
                               ObjectInfo *base) {

  auto *user = cast<llvm::Instruction>(use.getUser());
  auto *used = use.get();

  auto &module = MEMOIR_SANITIZE(user->getModule(), "User has no module.");
  const auto &data_layout = module.getDataLayout();
  auto &context = user->getContext();

  auto &size_type = Type::get_size_type(data_layout);
  auto &llvm_size_type = *size_type.get_llvm_type(context);

  // Handle has operations separately.
  if (auto *has = into<HasInst>(user)) {
    if (is_last_index(&use, has->index_operands_end())) {
      // Construct an if-else block.
      // if (has(encoder, key))
      //   i = read(encoder, key)
      //   h = has(collection, i)
      // h' = PHI(h, false)

      auto &cond = candidate.has_value(builder, *used, base);
      auto *phi = builder.CreatePHI(cond.getType(), 2);

      // i' = PHI(i, undef)
      auto *index_phi = builder.CreatePHI(&llvm_size_type, 2);

      auto *then_terminator =
          llvm::SplitBlockAndInsertIfThen(&cond,
                                          phi,
                                          /* unreachable? */ false);

      user->moveBefore(then_terminator);

      builder.SetInsertPoint(user);

      auto &encoded = candidate.encode_value(builder, *used, base);

      update_use(use, encoded);

      user->replaceAllUsesWith(phi);

      auto *then_bb = then_terminator->getParent();
      auto *else_bb = cond.getParent();

      phi->addIncoming(user, then_bb);
      phi->addIncoming(llvm::ConstantInt::getFalse(context), else_bb);

      index_phi->addIncoming(&encoded, then_bb);
      index_phi->addIncoming(llvm::UndefValue::get(&llvm_size_type), else_bb);

      return *index_phi;
    }
  }

  // In the common case, update the use with the encoded value.
  auto &encoded = candidate.encode_value(builder, *used, base);

  update_use(use, encoded);

  return encoded;
}

static bool dominates(ProxyInsertion::GetDominatorTree get_domtree,
                      llvm::Value &def,
                      llvm::Use &use) {
  llvm::Function *def_func;
  auto *arg = dyn_cast<llvm::Argument>(&def);
  auto *inst = dyn_cast<llvm::Instruction>(&def);
  if (arg)
    def_func = arg->getParent();
  else if (inst)
    def_func = inst->getFunction();
  else
    return false;

  llvm::Function *use_func;
  auto *user = dyn_cast<llvm::Instruction>(use.getUser());
  if (user)
    use_func = user->getFunction();
  else
    return false;

  // Must share parent function to dominate.
  if (def_func != use_func)
    return false;

  // Arguments dominate all instructions in the body.
  if (arg)
    return true;

  auto &domtree = get_domtree(*def_func);
  return domtree.dominates(inst, user);
}

static void encode_candidate_uses(
    Candidate &candidate,
    Set<llvm::Use *> &handled,
    ProxyInsertion::GetDominatorTree get_domtree) {

  // Patch the uses.
  Map<ObjectInfo *, Map<llvm::Value *, Set<llvm::Value *>>> values_added;
  for (const auto &[base, func_uses] : candidate.to_addkey)
    for (const auto &[func, uses] : func_uses)
      for (auto *use : uses) {
        if (handled.contains(use))
          continue;
        else
          handled.insert(use);

        auto *insert_point = insertion_point(*use);
        MemOIRBuilder builder(insert_point);

        auto *value = use->get();
        auto &added = candidate.add_value(builder, *value, base);
        update_use(*use, added);

        values_added[base][value].insert(&added);
      }

  for (const auto &[base, func_uses] : candidate.to_encode)
    for (const auto &[func, uses] : func_uses)
      for (auto *use : uses) {
        if (handled.contains(use))
          continue;
        else
          handled.insert(use);

        auto *value = use->get();
        auto *insert_point = insertion_point(*use);

        // Does a dominating addkey exist?
        bool already_added = false;
        for (auto *added : values_added[base][value]) {
          if (dominates(get_domtree, *added, *use)) {
            update_use(*use, *added);
            already_added = true;
            break;
          }
        }
        if (already_added)
          continue;

        MemOIRBuilder builder(insert_point);

        auto &encoded = encode_use(candidate, builder, *use, base);
      }
}

static void encode_uses(Vector<Candidate> &candidates,
                        ProxyInsertion::GetDominatorTree get_domtree) {
  Set<llvm::Use *> handled = {};
  for (auto &candidate : candidates)
    encode_candidate_uses(candidate, handled, get_domtree);
}

using ParamTypes =
    OrderedMultiMap<llvm::Function *, Pair<llvm::Argument *, llvm::Type *>>;
using AllocTypes = AssocList<AllocInst *, Type *>;
static void mutate_param_types(ParamTypes &params_to_mutate,
                               AllocTypes &allocs_to_mutate) {

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
    println("MUTATE PARAMS OF ", func->getName());
    for (; it != params_to_mutate.upper_bound(func); ++it) {
      auto [arg, type] = it->second;
      auto arg_idx = arg->getArgNo();

      param_types[arg_idx] = type;

      println("  ARG ", *arg, " TO ", *type);
    }

    // Create the new function type.
    auto *new_func_type = llvm::FunctionType::get(func_type->getReturnType(),
                                                  param_types,
                                                  func_type->isVarArg());

    // TODO: Fix the attributes on the new function.

    // Create the empty function to clone into.
    auto &new_func = MEMOIR_SANITIZE(llvm::Function::Create(new_func_type,
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

    // Clean up the old function.
    to_cleanup.insert(func);
  }

  // Mutate the types in the program.
  auto mutate_it = allocs_to_mutate.begin(), mutate_ie = allocs_to_mutate.end();
  for (; mutate_it != mutate_ie; ++mutate_it) {
    auto *alloc = mutate_it->first;
    auto *type = mutate_it->second;

    // Mutate the type.
    mutate_type(*alloc,
                *type,
                [&](llvm::Function &old_func,
                    llvm::Function &new_func,
                    llvm::ValueToValueMapTy &vmap) {
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

static void collect_types_to_mutate(Candidate &candidate,
                                    ParamTypes &params_to_mutate,
                                    AllocTypes &allocs_to_mutate) {

  // Fetch relevant type information.
  auto &module = candidate.module();
  auto &data_layout = module.getDataLayout();
  auto &context = module.getContext();

  auto &size_type = Type::get_size_type(data_layout);
  auto &llvm_size_type = *size_type.get_llvm_type(context);

  // Collect any function parameters whose type has changed because the
  // argument propagates an encoded value.
  for (const auto &[base, func_values] : candidate.encoded)
    for (const auto &[func, values] : func_values)
      for (auto *val : values)
        if (auto *arg = dyn_cast<llvm::Argument>(val))
          if (arg->getType() != &llvm_size_type)
            params_to_mutate.emplace(arg->getParent(),
                                     make_pair(arg, &llvm_size_type));

  // Collect the types that each candidate needs to be mutated to.
  for (auto *info : candidate) {
    auto *base = dyn_cast<BaseObjectInfo>(info);
    if (not base)
      continue;
    auto *alloc = &base->allocation();

    // Initialize the type to mutate if it doesnt exist already.
    auto found = allocs_to_mutate.find(alloc);
    if (found == allocs_to_mutate.end()) {
      allocs_to_mutate[alloc] = &alloc->getType();
    }
    auto &type = allocs_to_mutate[alloc];

#if 0
    // If the object is a total proxy, update it to be a sequence.
    if (is_total_proxy(*info, candidate.to_addkey)
        and not disable_total_proxy) {

      println(Style::BOLD, Colors::GREEN, "FOUND TOTAL PROXY", Style::RESET);

      type = &convert_to_sequence_type(*type, info->offsets());

    } else
#endif
    // Convert the type at the given offset to the size type.
    type = &convert_element_type(*type, info->offsets(), size_type);
  }
}

static bool check_initialized(llvm::Value &ptr) {
  // Ensure that the pointer has been initialized.
  for (auto &use : ptr.uses()) {
    auto *store = dyn_cast<llvm::StoreInst>(use.getUser());
    if (store and &ptr == store->getPointerOperand()) {
      return true;
    }
  }
  return false;
}

static void validate_mapping(Mapping &mapping) {
  for (const auto &[base, global] : mapping.globals()) {
    MEMOIR_ASSERT(check_initialized(*global),
                  "Global for ",
                  *base,
                  "not initialized!");
  }

  for (const auto &[base, local] : mapping.locals()) {
    MEMOIR_ASSERT(check_initialized(*local),
                  "Local for ",
                  *base,
                  "not initialized!");
  }
}

static bool is_self_recursive(llvm::Function &function) {
  for (auto &use : function.uses()) {
    auto *call = dyn_cast<llvm::CallBase>(use.getUser());
    if (not call) {
      continue;
    }

    if (call->isIndirectCall()) {
      continue;
    }

    if (call->getCalledFunction() == &function) {
      return true;
    }
  }

  return false;
}

static BaseObjectInfo *find_base(Candidate &candidate) {
  for (const auto &[obj, _] : candidate.equiv)
    if (auto *base = dyn_cast<BaseObjectInfo>(obj))
      return base;
  MEMOIR_UNREACHABLE("Failed to find allocation base");
}

static Pair<BaseObjectInfo *, ArgObjectInfo *> find_recursive_base(
    Candidate &candidate) {
  auto *base = find_base(candidate);
  auto *func = base->function();
  for (const auto &[obj, _] : candidate.equiv)
    if (auto *arg = dyn_cast<ArgObjectInfo>(obj))
      if (arg->function() == func)
        return { base, arg };
  return { base, NULL };
}

static void store_allocation(Candidate &candidate,
                             MemOIRBuilder &builder,
                             llvm::Value *encoder,
                             llvm::Value *decoder) {

  auto *function = builder.GetInsertBlock()->getParent();
  MEMOIR_ASSERT(function, "Builder has no parent function!");

  for (auto *info : candidate) {
    if (!isa<BaseObjectInfo>(info))
      continue;
    if (info->function() != function)
      continue;

    println("STORE ", *info);

    if (encoder) {
      auto &enc_global = candidate.encoder.global(*info);
      builder.CreateStore(encoder, &enc_global);

      auto &enc_local = candidate.encoder.local(*info);
      builder.CreateStore(encoder, &enc_local);
    }

    if (decoder) {
      auto &dec_global = candidate.decoder.global(*info);
      builder.CreateStore(decoder, &dec_global);

      auto &dec_local = candidate.decoder.local(*info);
      builder.CreateStore(decoder, &dec_local);
    }
  }
}

static void allocate_mappings(
    Candidate &candidate,
    std::function<llvm::DominatorTree &(llvm::Function &)> get_domtree) {

  infoln("PROXYING ", candidate);

  // Find the construction point for the candidate.
  auto &function = candidate.function();
  auto &domtree = get_domtree(function);
  auto &construction_point = candidate.construction_point(domtree);

  // Fetch LLVM context.
  auto &context = construction_point.getContext();
  auto &module = MEMOIR_SANITIZE(construction_point.getModule(),
                                 "Construction point has no module.");
  auto &data_layout = module.getDataLayout();

  // Allocate the proxy.
  MemOIRBuilder builder(&construction_point);

  // Fetch type information.
  auto &key_type = candidate.key_type();
  auto &size_type = Type::get_size_type(data_layout);
  auto size_type_bitwidth = size_type.getBitWidth();

  // Determine which proxies we need.
  bool build_encoder = candidate.build_encoder();
  bool build_decoder = candidate.build_decoder();

#if 0
  // If the construction function is self recursive, we will conditionally
  // re-use the input mappings.
  auto *recursive_base = find_recursive_base(candidate);

  // If we are in a recursive function, condition allocation on a heuristic.
  if (recursive_base) {

    // Load the mappings from the stack.
    llvm::Value *old_encoder = NULL, *old_decoder = NULL;
    if (build_encoder) {
      auto &local = candidate.encoder.local(recursive_base);
      old_encoder =
          builder.CreateLoad(local.getAllocatedType(), &local, "enc.old.");
    }

    if (build_decoder) {
      auto &local = candidate.decoder.local(recursive_base);
      old_decoder =
          builder.CreateLoad(local.getAllocatedType(), &local, "dec.old.");
    }

    // Check if the old mappings are nonnull.
    llvm::Value *nonnull = NULL;
    if (old_encoder) {
      nonnull = builder.CreateICmpNE(
          old_encoder,
          llvm::Constant::getNullValue(old_encoder->getType()));
    }

    if (old_decoder) {
      auto *dec_nonnull = builder.CreateICmpNE(
          old_decoder,
          llvm::Constant::getNullValue(old_decoder->getType()));
      if (nonnull) {
        nonnull = builder.CreateAnd(nonnull, dec_nonnull);
      } else {
        nonnull = dec_nonnull;
      }
    }

    // Create an if-then-else region conditioned on the old mappings being
    // non-null.
    // AFTER:
    // br nonnull ? then_block : else_block
    // then_block: br cont_block
    // else_block: br cont_block
    llvm::Instruction *then_term, *else_term;
    llvm::SplitBlockAndInsertIfThenElse(nonnull,
                                        &construction_point,
                                        &then_term,
                                        &else_term);
    auto *then_block = then_term->getParent();
    auto *else_block = else_term->getParent();

    // TODO: call the heuristic function to determine if we should reuse the old
    // mappings or not.

    // In the then block, store the old mappings to the locals.
    builder.SetInsertPoint(then_term);
    store_allocation(candidate, builder, old_encoder, old_decoder);

    // Set the builder to the else block so we can allocate the encoder/decoder.
    builder.SetInsertPoint(else_term);
  }
#endif

  // Allocate the encoder.
  llvm::Instruction *encoder = nullptr;
  if (build_encoder) {
    auto *encoder_alloc =
        builder.CreateAllocInst(candidate.encoder_type(), {}, "enc.new.");
    encoder = &encoder_alloc->getCallInst();
  }

  // Allocate the decoder.
  llvm::Instruction *decoder = nullptr;
  Type *decoder_type = nullptr;
  if (build_decoder) {
    auto *decoder_alloc = builder.CreateAllocInst(candidate.decoder_type(),
                                                  { builder.getInt64(0) },
                                                  "dec.new.");
    decoder = &decoder_alloc->getCallInst();
  }

  // Store the allocated mappings to the relevant globals.
  store_allocation(candidate, builder, encoder, decoder);

  // Validate that the mappings are initialized correctly.
  validate_mapping(candidate.encoder);
  validate_mapping(candidate.decoder);
  println("VALIDATED MAPPINGS");
}

bool ProxyInsertion::transform() {

  bool modified = true;

  // Track the function parameters whose type needs to be mutated.
  ParamTypes params_to_mutate = {};
  AssocList<AllocInst *, Type *> allocs_to_mutate;

  // Transform the program for each candidate.
  for (auto &candidate : this->candidates) {
    allocate_mappings(candidate, this->get_dominator_tree);
  }

  // Patch uses to decode.
  decode_uses(candidates);

  // Patch uses to encode/addkey.
  encode_uses(candidates, this->get_dominator_tree);

  // Promote the locals to registers.
  for (auto &candidate : candidates) {
    promote(candidate, this->get_dominator_tree);
  }

  // Find parameter and allocation types that need to be mutated.
  for (auto &candidate : candidates) {
    collect_types_to_mutate(candidate, params_to_mutate, allocs_to_mutate);
  }

  // Mutate the type of function parameters in the program.
  mutate_param_types(params_to_mutate, allocs_to_mutate);

  return modified;
}

} // namespace folio
