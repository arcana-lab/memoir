#include <numeric>

#include "llvm/IR/AttributeMask.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "memoir/ir/Builder.hpp"
#include "memoir/ir/Instructions.hpp"
#include "memoir/raise/RepairSSA.hpp"
#include "memoir/support/AssocList.hpp"
#include "memoir/support/SortedVector.hpp"
#include "memoir/support/WorkList.hpp"
#include "memoir/transform/utilities/PromoteGlobals.hpp"
#include "memoir/utility/Metadata.hpp"

#include "DataEnumeration.hpp"
#include "Utilities.hpp"

using namespace memoir;

namespace memoir {

// Command line options.
static llvm::cl::opt<int> max_reuses(
    "ade-max-reuses",
    llvm::cl::desc("Maximum number of enumeration reuses"),
    llvm::cl::init(-1));

using GetGlobal = typename std::function<llvm::GlobalVariable *(ObjectInfo *)>;

static bool dominates(DataEnumeration::GetDominatorTree get_domtree,
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

static bool replace_with_dominator(
    DataEnumeration::GetDominatorTree get_domtree,
    llvm::ArrayRef<llvm::Instruction *> defs,
    llvm::Use &use) {
  for (auto *def : defs) {
    if (dominates(get_domtree, *def, use)) {
      use.set(def);
      return true;
    }
  }
  return false;
}

static llvm::Type *pointee_type(llvm::Value *ptr) {
  if (auto *global = dyn_cast<llvm::GlobalVariable>(ptr)) {
    return global->getValueType();
  } else if (auto *local = dyn_cast<llvm::AllocaInst>(ptr)) {
    return local->getAllocatedType();
  }
  MEMOIR_UNREACHABLE("Could not get pointee type for ", *ptr);
}

static llvm::Value *load_mapping(Builder &builder,
                                 Type &mapping_type,
                                 llvm::Value *ptr) {
  llvm::Twine name(ptr->getName());
  auto *val = builder.CreateLoad(pointee_type(ptr), ptr, name.concat(".local"));
  builder.CreateAssertTypeInst(val, mapping_type, name.concat(".type"));

  return val;
}

llvm::Value *DataEnumeration::load_encoder(Builder &builder,
                                           const TransformInfo &info) {
  return load_mapping(builder, *info.encoder_type, info.enc_global);
}

llvm::Value *DataEnumeration::load_decoder(Builder &builder,
                                           const TransformInfo &info) {
  return load_mapping(builder, *info.decoder_type, info.dec_global);
}

static void store_mapping(Builder &builder,
                          Type &mapping_type,
                          llvm::Value *ptr,
                          llvm::Value *val) {
  llvm::Twine name(ptr->getName());
  builder.CreateAssertTypeInst(val, mapping_type, name.concat(".type"));
  builder.CreateStore(val, ptr);
}

void DataEnumeration::store_encoder(Builder &builder,
                                    const TransformInfo &info,
                                    llvm::Value *enc) {
  return store_mapping(builder, *info.encoder_type, info.enc_global, enc);
}

void DataEnumeration::store_decoder(Builder &builder,
                                    const TransformInfo &info,
                                    llvm::Value *dec) {
  return store_mapping(builder, *info.decoder_type, info.dec_global, dec);
}

#ifndef PRINT_VALUES
#  define PRINT_VALUES 0
#endif

llvm::Instruction &DataEnumeration::has_value(Builder &builder,
                                              const TransformInfo &info,
                                              llvm::Value &value) {
#if PRINT_VALUES
  builder.CreateErrorf("HAS %u\n", { &value });
#endif
  auto *enc = this->load_encoder(builder, info);
  return builder.CreateHasInst(enc, { &value })->getCallInst();
};

llvm::Value &DataEnumeration::decode_value(Builder &builder,
                                           const TransformInfo &info,
                                           llvm::Value &value) {
#if PRINT_VALUES
  builder.CreateErrorf("DEC %lu ", { &value });
#endif

  auto *dec = this->load_decoder(builder, info);
  auto &decoded =
      builder.CreateReadInst(*info.key_type, dec, { &value })->asValue();

#if PRINT_VALUES
  builder.CreateErrorf("= %u\n", { &decoded });
#endif

  return decoded;
};

llvm::Value &DataEnumeration::encode_value(Builder &builder,
                                           const TransformInfo &info,
                                           llvm::Value &value) {
#if PRINT_VALUES
  builder.CreateErrorf("ENC %u ", { &value });
#endif

  auto *enc = this->load_encoder(builder, info);

  auto &size_type = Type::get_size_type(this->module.getDataLayout());
  auto &encoded = builder.CreateReadInst(size_type, enc, &value)->asValue();

#if PRINT_VALUES
  builder.CreateErrorf("= %lu\n", { &encoded });
#endif

  return encoded;
};

static llvm::Value *coerce(Builder &builder,
                           llvm::Value *value,
                           llvm::Type *new_type) {
  // FIXME: We shouldn't have to do this here. There should be a type coercion
  // pass in memoir that converts indices to match the key type.
  auto *old_type = value->getType();

  if (isa<llvm::IntegerType>(old_type) and isa<llvm::IntegerType>(new_type)) {
    if (isa<llvm::ConstantInt>(value)) {
      auto *old_const = dyn_cast<llvm::ConstantInt>(value);
      return llvm::ConstantInt::get(new_type, old_const->getZExtValue());
    }

    return builder.CreateZExtOrTrunc(value, new_type, ".coerce");
  }
  MEMOIR_UNREACHABLE(
      "NYI, but you should add type coercion instead of extending");
}

llvm::Value &DataEnumeration::add_value(Builder &builder,
                                        const TransformInfo &info,
                                        llvm::Value &value) {
#if PRINT_VALUES
  builder.CreateErrorf("ADD %u ", { &value });
#endif

  Vector<llvm::Value *> args = { &value };

  llvm::AllocaInst *enc_inout = NULL, *dec_inout = NULL;

  if (info.build_encoder) {
    // Fetch the current state of the encoder.
    auto *enc = this->load_encoder(builder, info);

    // Create an inout stack variable and initialize it.
    enc_inout = builder.CreateLocal(builder.getPtrTy());
    // builder.CreateLifetimeStart(enc_inout);
    builder.CreateStore(enc, enc_inout);

    // Pass the inout stack variable to the function.
    args.push_back(enc_inout);
  }
  if (info.build_decoder) {
    // Fetch the current state of the decoder.
    auto *dec = this->load_decoder(builder, info);

    // Create an inout stack variable and initialize it.
    dec_inout = builder.CreateLocal(builder.getPtrTy());
    // builder.CreateLifetimeStart(dec_inout);
    builder.CreateStore(dec, dec_inout);

    // Pass the inout stack variable to the function.
    args.push_back(dec_inout);
  }

  // Coerce the arguments to match the callee.
  auto callee = info.addkey_callee;
  auto *func_type = callee.getFunctionType();
  size_t index = 0;
  for (auto &arg : args) {
    auto *arg_type = arg->getType();
    auto *param_type = func_type->getParamType(index);
    if (arg_type != param_type)
      arg = coerce(builder, arg, param_type);

    ++index;
  }

  // Call the addkey function.
  auto &encoded = MEMOIR_SANITIZE(builder.CreateCall(callee, args),
                                  "Failed to create call to addkey!");

  // Load the inout results.
  if (enc_inout) {
    auto *enc = builder.CreateLoad(builder.getPtrTy(), enc_inout);
    // builder.CreateLifetimeEnd(enc_inout);
    this->store_encoder(builder, info, enc);
  }
  if (dec_inout) {
    auto *dec = builder.CreateLoad(builder.getPtrTy(), dec_inout);
    // builder.CreateLifetimeEnd(dec_inout);
    this->store_decoder(builder, info, dec);
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

void DataEnumeration::promote() {

  // Collect all of the globals that can be promoted.
  Vector<llvm::GlobalVariable *> globals_to_promote;
  for (const auto &[parent, info] : this->to_transform) {
    auto *enc = info.enc_global;
    if (enc and global_is_promotable(*enc))
      globals_to_promote.push_back(info.enc_global);

    auto *dec = info.dec_global;
    if (dec and global_is_promotable(*dec))
      globals_to_promote.push_back(info.dec_global);
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

void DataEnumeration::decode_uses() {
  Set<llvm::Use *> handled = {};
  for (const auto &[base, info] : this->to_transform) {
    debugln(" >> CANDIDATE : ", *info.key_type, " << ");

    for (const auto &[func, uses] : info.to_decode) {
      Map<llvm::Value *, Vector<llvm::Instruction *>> decoded_values;
      for (auto *use : uses) {
        if (handled.contains(use))
          continue;
        else
          handled.insert(use);

        auto *used = use->get();
        auto *user = dyn_cast<llvm::Instruction>(use->getUser());

        // Does a dominating decode for this value exist?
        if (replace_with_dominator(this->get_dominator_tree,
                                   decoded_values[used],
                                   *use))
          continue;

        // Compute the insertion point.
        Builder builder(insertion_point(*use));

        auto &decoded = this->decode_value(builder, info, *used);

        if (auto *inst = dyn_cast<llvm::Instruction>(&decoded))
          decoded_values[used].push_back(inst);

        debugln("  DECODED ", decoded);

        use->set(&decoded);
      }
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
}

llvm::Value &DataEnumeration::encode_use(Builder &builder,
                                         const TransformInfo &info,
                                         llvm::Use &use) {

  debugln("ENCODE ", pretty_use(use));

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

      auto &cond = this->has_value(builder, info, *used);
      auto *phi = builder.CreatePHI(cond.getType(), 2);

      // i' = PHI(i, undef)
      auto *index_phi = builder.CreatePHI(&llvm_size_type, 2);

      auto *then_terminator =
          llvm::SplitBlockAndInsertIfThen(&cond,
                                          phi,
                                          /* unreachable? */ false);

      user->moveBefore(then_terminator);

      builder.SetInsertPoint(user);

      auto &encoded = this->encode_value(builder, info, *used);

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
  auto &encoded = this->encode_value(builder, info, *used);

  update_use(use, encoded);

  return encoded;
}

void DataEnumeration::patch_uses() {

  Set<llvm::Use *> handled = {};
  debugln("=== PATCH USES === ");

  this->decode_uses();

  for (const auto &[base, info] : to_transform) {
    debugln(" >> EQUIV CLASS << ");

    Map<llvm::Value *, Vector<llvm::Instruction *>> values_added;
    for (const auto &[func, uses] : info.to_addkey)
      for (auto *use : uses) {
        if (handled.contains(use))
          continue;
        else
          handled.insert(use);

        auto *used = use->get();

        if (replace_with_dominator(this->get_dominator_tree,
                                   values_added[used],
                                   *use))
          continue;

        auto *insert_point = insertion_point(*use);
        Builder builder(insert_point);

        auto &added = add_value(builder, info, *used);
        update_use(*use, added);

        debugln("  ADDED ", added);

        if (auto *inst = dyn_cast<llvm::Instruction>(&added))
          values_added[used].push_back(inst);
      }

    Map<llvm::Value *, Vector<llvm::Instruction *>> values_encoded;
    for (const auto &[func, uses] : info.to_encode)
      for (auto *use : uses) {
        if (handled.contains(use))
          continue;
        else
          handled.insert(use);

        auto *used = use->get();
        auto *insert_point = insertion_point(*use);

        if (not into<HasInst>(use->getUser())) {
          // Does a dominating addkey exist?
          if (values_added.contains(used))
            if (replace_with_dominator(this->get_dominator_tree,
                                       values_added.at(used),
                                       *use))
              continue;

          // Does a dominating encode exist?
          if (replace_with_dominator(this->get_dominator_tree,
                                     values_encoded[used],
                                     *use))
            continue;
        }

        Builder builder(insert_point);

        auto &encoded = this->encode_use(builder, info, *use);

        if (auto *inst = dyn_cast<llvm::Instruction>(&encoded))
          values_encoded[used].push_back(inst);

        for (auto &use : encoded.uses())
          handled.insert(&use);
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

static void store_allocation(Builder &builder,
                             llvm::Value *encoder,
                             llvm::Value *decoder,
                             llvm::Value *enc_ptr,
                             llvm::Value *dec_ptr) {

  auto *function = builder.GetInsertBlock()->getParent();
  MEMOIR_ASSERT(function, "Builder has no parent function!");

  if (encoder)
    builder.CreateStore(encoder, enc_ptr);

  if (decoder)
    builder.CreateStore(decoder, dec_ptr);
}

static llvm::Instruction &find_construction_point(
    llvm::DominatorTree &domtree,
    llvm::ArrayRef<ObjectInfo *> objects) {
  // Find the construction point for the encoder and decoder.
  llvm::Instruction *construction_point = NULL;

  // Find a point that dominates all of the object allocations.
  for (auto *obj : objects) {
    auto *base = dyn_cast<BaseObjectInfo>(obj);
    if (not base)
      continue;

    auto *inst = &base->allocation().getCallInst();

    construction_point =
        construction_point
            ? domtree.findNearestCommonDominator(construction_point, inst)
            : inst;
  }

  return MEMOIR_SANITIZE(
      construction_point,
      "Failed to find a construction point for the candidate!");
}

ObjectInfo *DataEnumeration::find_recursive_base(BaseObjectInfo &base) {
  // Is there an argument base in the same function as this one?
  // If so, find the one corresponding to this base.
  auto *function = base.function();

  for (const auto &[other, _] : this->equiv) {
    if (other->function() != function)
      continue;

    auto *arg = dyn_cast<ArgObjectInfo>(other);
    if (!arg)
      continue;

    for (const auto &[call, incoming] : arg->incoming())
      if (this->unified.find(incoming) == this->unified.find(&base))
        return arg;
  }

  return NULL;
}

static llvm::Value *construct_reuse_heuristic(MemOIRBuilder &builder,
                                              llvm::Function &function,
                                              llvm::Value *nonnull) {

  // If the max number of reuses is negative, then there is no max.
  if (max_reuses < 0)
    return nonnull;

  // If the max number of reuses is zero, then never reuse.
  if (max_reuses == 0)
    return builder.getFalse();

  // Add a global variable to track the current recursion depth.
  auto *zero = builder.getInt32(0);
  auto *counter_type = zero->getType();
  auto *global =
      new llvm::GlobalVariable(builder.getModule(),
                               counter_type,
                               /*constant?*/ false,
                               llvm::GlobalValue::LinkageTypes::InternalLinkage,
                               zero,
                               "reuse");

  { // Increment at the beginning of the function, and decrement at each return.
    auto checkpoint = builder.saveIP();

    auto &entry = function.getEntryBlock();
    builder.SetInsertPoint(entry.getFirstInsertionPt());

    // Increment the global.
    auto *load = builder.CreateLoad(counter_type, global);
    auto *plus_one = builder.CreateAdd(load, builder.getInt32(1), "reuse.add");
    builder.CreateStore(plus_one, global);

    for (auto &block : function) {
      auto *ret = dyn_cast<llvm::ReturnInst>(block.getTerminator());
      if (!ret)
        continue;
      builder.SetInsertPoint(ret);

      // Decrement the global.
      auto *load = builder.CreateLoad(counter_type, global);
      auto *minus_one =
          builder.CreateSub(load, builder.getInt32(1), "reuse.sub");
      builder.CreateStore(minus_one, global);
    }

    builder.restoreIP(checkpoint);
  }

  // Load the global variable.
  auto *load = builder.CreateLoad(counter_type, global);

  // Check if the global is equal to the max number of reuses.
  //   0 < count and count % max == 0
  auto *gt_zero = builder.CreateICmpEQ(load, zero);
  auto *max = builder.getInt32(max_reuses);
  auto *rem = builder.CreateURem(load, max);
  auto *rem_is_zero = builder.CreateICmpUGT(rem, zero, "reuse.cmp");
  auto *cond = builder.CreateOr(gt_zero, rem_is_zero);

  // Compute the heuristic as: and(is_max, nonnull)
  auto *heuristic = builder.CreateAnd(cond, nonnull, "reuse.cond");

  return heuristic;
}

void DataEnumeration::allocate_mappings(BaseObjectInfo &base) {

  const auto &equiv = this->equiv.at(&base);
  const auto &info = this->to_transform.at(&base);

  // Find the construction point for the info.
  auto &function =
      MEMOIR_SANITIZE(base.function(), "Expected object with function parent");
  auto &domtree = get_dominator_tree(function);
  auto &construction_point = find_construction_point(domtree, equiv);

  debugln("        BASE ", base);
  debugln("          IN ", construction_point.getFunction()->getName());
  debugln("CONSTRUCT AT ", construction_point);

  // Fetch LLVM context.
  auto &context = construction_point.getContext();
  auto &module = MEMOIR_SANITIZE(construction_point.getModule(),
                                 "Construction point has no module.");
  auto &data_layout = module.getDataLayout();

  // Allocate the enumeration.
  Builder builder(&construction_point);

  // Fetch type information.
  auto &key_type = *info.key_type;
  auto &size_type = Type::get_size_type(data_layout);
  auto size_type_bitwidth = size_type.getBitWidth();

  // Determine which proxies we need.
  bool build_encoder = info.build_encoder;
  bool build_decoder = info.build_decoder;

  // If the construction function is self recursive, we will conditionally
  // re-use the input mappings.
  auto *recursive_base = this->find_recursive_base(base);

  // If we are in a recursive function, condition allocation on a heuristic.
  if (recursive_base) {

    debugln("RECURSIVE BASE FOUND");
    debugln("  BASE ", base);
    debugln("   ARG ", *recursive_base);

    const auto &rec_info = this->to_transform.at(recursive_base);

    // Load the mappings from the stack.
    llvm::Value *old_encoder = NULL, *old_decoder = NULL;
    if (build_encoder)
      old_encoder = this->load_encoder(builder, rec_info);

    if (build_decoder)
      old_decoder = this->load_decoder(builder, rec_info);

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

    // If the user has specified a max number of reuses, do so with a global
    // variable.
    auto *heuristic = construct_reuse_heuristic(builder, function, nonnull);

    // Create an if-then-else region conditioned on the old mappings being
    // non-null.
    // AFTER:
    // br nonnull ? then_block : else_block
    // then_block: br cont_block
    // else_block: br cont_block
    llvm::Instruction *then_term, *else_term;
    llvm::SplitBlockAndInsertIfThenElse(heuristic,
                                        &construction_point,
                                        &then_term,
                                        &else_term);
    auto *then_block = then_term->getParent();
    auto *else_block = else_term->getParent();

    // TODO: call the heuristic function to determine if we should reuse the old
    // mappings or not.

    // In the then block, store the old mappings to the locals.
    builder.SetInsertPoint(then_term);
    store_allocation(builder,
                     old_encoder,
                     old_decoder,
                     info.enc_global,
                     info.dec_global);

    // Set the builder to the else block so we can allocate the encoder/decoder.
    builder.SetInsertPoint(else_term);
  }

  // Allocate the encoder.
  llvm::Instruction *encoder = nullptr;
  if (build_encoder) {
    auto *encoder_alloc =
        builder.CreateAllocInst(*info.encoder_type, {}, "enc.new.");
    encoder = &encoder_alloc->getCallInst();
  }

  // Allocate the decoder.
  llvm::Instruction *decoder = nullptr;
  Type *decoder_type = nullptr;
  if (build_decoder) {
    auto *decoder_alloc = builder.CreateAllocInst(*info.decoder_type,
                                                  { builder.getInt64(0) },
                                                  "dec.new.");
    decoder = &decoder_alloc->getCallInst();
  }

  // Store the allocated mappings to the relevant globals.
  store_allocation(builder, encoder, decoder, info.enc_global, info.dec_global);
}

bool DataEnumeration::transform() {

  bool modified = true;

  // Transform the program for each candidate.
  debugln("=== ALLOCATE MAPPINGS ===");
  for (auto &[parent, info] : this->to_transform) {
    if (auto *base = dyn_cast<BaseObjectInfo>(parent))
      allocate_mappings(*base);
  }
  debugln();

  // Patch uses to encode/addkey.
  debugln("=== PATCH USES TO ENCODE/ADDKEY ===");
  this->patch_uses();
  debugln();

  // Promote the locals to registers.
  debugln("=== PROMOTE GLOBALS ===");
  this->promote();
  debugln();

  // Mutate the type of function parameters in the program.
  debugln("=== MUTATE PARAM TYPES ===");
  this->mutate_types();
  debugln();

  return modified;
}

} // namespace memoir
