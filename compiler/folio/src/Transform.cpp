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

static void decode_candidate_uses(Candidate &candidate,
                                  Set<llvm::Use *> &handled) {

  for (const auto &[base, uses] : candidate.to_decode) {
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
}

static void decode_uses(Vector<Candidate> &candidates) {
  Set<llvm::Use *> handled = {};
  for (auto &candidate : candidates) {
    decode_candidate_uses(candidate, handled);
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

static llvm::Value &encode_use(Candidate &candidate,
                               MemOIRBuilder &builder,
                               llvm::Use &use,
                               llvm::Value *base) {

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

static void encode_candidate_uses(Candidate &candidate,
                                  Set<llvm::Use *> &handled) {

  for (const auto &[base, uses] : candidate.to_addkey) {
    for (auto *use : uses) {
      if (handled.contains(use)) {
        continue;
      } else {
        handled.insert(use);
      }

      MemOIRBuilder builder(insertion_point(*use));

      auto &added = candidate.add_value(builder, *use->get(), base);
      update_use(*use, added);
    }
  }

  for (const auto &[base, uses] : candidate.to_encode) {
    for (auto *use : uses) {
      if (handled.contains(use)) {
        continue;
      } else {
        handled.insert(use);
      }

      MemOIRBuilder builder(insertion_point(*use));

      auto &encoded = encode_use(candidate, builder, *use, base);
    }
  }
}

static void encode_uses(Vector<Candidate> &candidates) {
  Set<llvm::Use *> handled = {};
  for (auto &candidate : candidates) {
    encode_candidate_uses(candidate, handled);
  }
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
  for (const auto &[func, base_to_values] : candidate.encoded_values) {
    for (const auto &[base, values] : base_to_values) {
      for (auto *val : values) {
        if (auto *arg = dyn_cast<llvm::Argument>(val)) {
          if (arg->getType() != &llvm_size_type) {
            params_to_mutate.emplace(arg->getParent(),
                                     make_pair(arg, &llvm_size_type));
          }
        }
      }
    }
  }

  // Collect the types that each candidate needs to be mutated to.
  for (auto *info : candidate) {
    auto *alloc = info->allocation;

    // Initialize the type to mutate if it doesnt exist already.
    auto found = allocs_to_mutate.find(alloc);
    if (found == allocs_to_mutate.end()) {
      allocs_to_mutate[alloc] = &alloc->getType();
    }
    auto &type = allocs_to_mutate[alloc];

    // If the object is a total proxy, update it to be a sequence.
    if (is_total_proxy(*info, candidate.added) and not disable_total_proxy) {

      println(Style::BOLD, Colors::GREEN, "FOUND TOTAL PROXY", Style::RESET);

      type = &convert_to_sequence_type(*type, info->offsets);

    } else {
      // Convert the type at the given offset to the size type.
      type = &convert_element_type(*type, info->offsets, size_type);
    }
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

static llvm::Value *find_recursive_base(Candidate &candidate) {
  llvm::Value *recursive_base = NULL;

  auto &function = candidate.function();

  if (is_self_recursive(function)) {
    // TODO: attach a boolean to specify that the top of the call stack is a
    // self-recursive call.

    // Find the base encoder argument for the calling context.
    // TODO: make this a little nicer,
    for (const auto &to_patch :
         { candidate.to_decode, candidate.to_encode, candidate.to_addkey }) {

      if (recursive_base) {
        // TODO: ensure that this base is the same for all required mappings,
        // otherwise they might end up using different states.
      }

      for (const auto &[base, uses] : to_patch) {
        if (auto *arg = dyn_cast<llvm::Argument>(base)) {
          if (arg->getParent() == &function) {
            recursive_base = base;
            break;
          }
        }
      }
    }
  }

  return recursive_base;
}

static void store_allocation(Candidate &candidate,
                             MemOIRBuilder &builder,
                             llvm::Value *encoder,
                             llvm::Value *decoder) {

  for (auto *info : candidate) {
    auto *alloc = &info->allocation->asValue();

    if (encoder) {
      auto &enc_global = candidate.encoder.global(alloc);
      builder.CreateStore(encoder, &enc_global);

      auto &enc_local = candidate.encoder.local(alloc);
      builder.CreateStore(encoder, &enc_local);
    }

    if (decoder) {
      auto &dec_global = candidate.decoder.global(alloc);
      builder.CreateStore(decoder, &dec_global);

      auto &dec_local = candidate.decoder.local(alloc);
      builder.CreateStore(decoder, &dec_local);
    }
  }
}

static void allocate_mappings(
    Candidate &candidate,
    std::function<llvm::DominatorTree &(llvm::Function &)> get_domtree) {

  // Unpack the candidate.
  auto &encoded = candidate.encoded;
  auto &decoded = candidate.decoded;
  auto &added = candidate.added;

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
  encode_uses(candidates);

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
