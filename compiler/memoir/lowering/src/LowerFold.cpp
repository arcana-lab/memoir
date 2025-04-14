// LLVM
#include "llvm/IR/CFG.h"

#include "llvm/Transforms/Utils/BasicBlockUtils.h"

// MEMOIR
#include "memoir/ir/Builder.hpp"

#include "memoir/support/Casting.hpp"
#include "memoir/support/Print.hpp"

#include "memoir/transforms/utilities/Inlining.hpp"

#include "memoir/lowering/LowerFold.hpp"
namespace llvm::memoir {

namespace detail {

struct HeaderInfo {
  HeaderInfo(llvm::BasicBlock &preheader,
             llvm::BasicBlock &header,
             llvm::BasicBlock &body,
             llvm::BasicBlock &exit,
             llvm::PHINode &accumulator,
             llvm::Value &key,
             llvm::Value *element)
    : preheader(preheader),
      header(header),
      body(body),
      exit(exit),
      accumulator(accumulator),
      key(key),
      element(element) {}

  llvm::BasicBlock &preheader;
  llvm::BasicBlock &header;
  llvm::BasicBlock &body;
  llvm::BasicBlock &exit;
  llvm::PHINode &accumulator;
  llvm::Value &key;
  llvm::Value *element;
};

static HeaderInfo lower_fold_header(FoldInst &I,
                                    MemOIRBuilder &builder,
                                    llvm::Value &collection,
                                    CollectionType &collection_type,
                                    llvm::Function &begin_func,
                                    llvm::Function &next_func,
                                    llvm::Type &iter_type) {

  // Split the block before the fold instruction, creating a while loop.
  //   it = alloca
  //   elem = alloca
  //   begin(it, collection)
  //   while (next(it, elem))
  //     {body}
  //   {continuation}
  auto &inst = I.getCallInst();
  auto *preheader =
      llvm::SplitBlock(inst.getParent(),
                       &inst,
                       /* DomTreeUpdater = */ (llvm::DomTreeUpdater *)nullptr,
                       /* LoopInfo = */ nullptr,
                       /* MemorySSAUpdater = */ nullptr,
                       /* BBName = */ "fold.loop.preheader.",
                       /* Before = */ false);
  auto *body =
      llvm::SplitBlock(inst.getParent(),
                       &inst,
                       /* DomTreeUpdater = */ (llvm::DomTreeUpdater *)nullptr,
                       /* LoopInfo = */ nullptr,
                       /* MemorySSAUpdater = */ nullptr,
                       /* BBName = */ "fold.loop.body.",
                       /* Before = */ false);
  auto *header = llvm::BasicBlock::Create(builder.getContext(),
                                          "fold.loop.header.",
                                          body->getParent(),
                                          body);
  auto *exit =
      llvm::SplitBlock(inst.getParent(),
                       &inst,
                       /* DomTreeUpdater = */ (llvm::DomTreeUpdater *)nullptr,
                       /* LoopInfo = */ nullptr,
                       /* MemorySSAUpdater = */ nullptr,
                       /* BBName = */ "fold.loop.exit.",
                       /* Before = */ false);

  // Rewire the branch target of the preheader to the header.
  auto *preheader_branch =
      dyn_cast<llvm::BranchInst>(preheader->getTerminator());
  preheader_branch->setSuccessor(0, header);

  // Construct the contents of the preheader.
  builder.SetInsertPoint(preheader->getFirstInsertionPt());

  // Insert the iterator and element allocations.
  auto *iter_alloca = builder.CreateAlloca(&iter_type);

  // Insert the call to begin.
  auto begin_callee = FunctionCallee(&begin_func);
  auto *iterator = builder.CreateCall(
      begin_callee,
      llvm::ArrayRef<llvm::Value *>({ iter_alloca, &collection }));

  // Construct the contents of the header.
  builder.SetInsertPoint(header);

  // Create a PHI node for the accumulator value.
  auto *accumulator_llvm_type = I.getCallInst().getType();
  auto &accumulator = MEMOIR_SANITIZE(
      builder.CreatePHI(accumulator_llvm_type, 2, "fold.accum.phi."),
      "Unable to create PHI node for accumulator!");

  // Insert the call to next.
  auto next_callee = FunctionCallee(&next_func);
  auto *has_next =
      builder.CreateCall(next_callee,
                         llvm::ArrayRef<llvm::Value *>({ iter_alloca }));

  // Replace the current terminator with a conditional branch based on the
  // result of next.
  builder.CreateCondBr(has_next, body, exit);

  // Update the builder's insertion point to the body of the loop.
  builder.SetInsertPoint(body->getFirstInsertionPt());

  // Rewire the branch target of the body's terminator to be the header.
  auto *body_branch = dyn_cast<llvm::BranchInst>(body->getTerminator());
  body_branch->setSuccessor(0, header);

  // Unpack the lowered element type.
  auto &iter_struct_type =
      MEMOIR_SANITIZE(dyn_cast<llvm::StructType>(&iter_type),
                      "Lowered iter type is not a struct!");
  auto *key_type = iter_struct_type.getElementType(0);

  // Load the key.
  auto *key_gep =
      builder.CreateStructGEP(&iter_type, iter_alloca, 0, "fold.key.ptr.");
  auto *key = builder.CreateLoad(key_type, key_gep, "fold.key.");

  // Load the value, if it exists.
  llvm::Value *element = nullptr;
  auto &element_type = collection_type.getElementType();
  if (not isa<VoidType>(&element_type)) {
    auto *element_gep =
        builder.CreateStructGEP(&iter_type, iter_alloca, 1, "fold.elem.ptr.");

    auto *llvm_element_type = iter_struct_type.getElementType(1);
    MEMOIR_NULL_CHECK(llvm_element_type,
                      "LLVM struct type has no type for second field!");
    element = builder.CreateLoad(llvm_element_type, element_gep, "fold.elem.");

    // If the resultant is a boolean, we'll need to cast it.
    if (auto *integer_type = dyn_cast<IntegerType>(&element_type)) {
      if (integer_type->getBitWidth() == 1) {
        element = builder.CreateTrunc(element, builder.getInt1Ty());
      }
    }
  }

  return HeaderInfo(*preheader,
                    *header,
                    *body,
                    *exit,
                    accumulator,
                    *key,
                    element);
}

static HeaderInfo lower_fold_header(
    FoldInst &I,
    MemOIRBuilder &builder,
    llvm::Value &collection,
    llvm::memoir::CollectionType &collection_type) {

  // Split the block before the fold instruction, creating a while loop.
  //   it = alloca
  //   elem = alloca
  //   begin(it, collection)
  //   while (next(it, elem))
  //     {body}
  //   {continuation}
  auto &inst = I.getCallInst();
  auto *preheader =
      llvm::SplitBlock(inst.getParent(),
                       &inst,
                       /* DomTreeUpdater = */ (llvm::DomTreeUpdater *)nullptr,
                       /* LoopInfo = */ nullptr,
                       /* MemorySSAUpdater = */ nullptr,
                       /* BBName = */ "fold.loop.preheader.",
                       /* Before = */ false);
  auto *body =
      llvm::SplitBlock(inst.getParent(),
                       &inst,
                       /* DomTreeUpdater = */ (llvm::DomTreeUpdater *)nullptr,
                       /* LoopInfo = */ nullptr,
                       /* MemorySSAUpdater = */ nullptr,
                       /* BBName = */ "fold.loop.body.",
                       /* Before = */ false);
  auto *header = llvm::BasicBlock::Create(builder.getContext(),
                                          "fold.loop.header.",
                                          body->getParent(),
                                          body);
  auto *exit =
      llvm::SplitBlock(inst.getParent(),
                       &inst,
                       /* DomTreeUpdater = */ (llvm::DomTreeUpdater *)nullptr,
                       /* LoopInfo = */ nullptr,
                       /* MemorySSAUpdater = */ nullptr,
                       /* BBName = */ "fold.loop.exit.",
                       /* Before = */ false);

  // Rewire the branch target of the preheader to the header.
  auto *preheader_branch =
      dyn_cast<llvm::BranchInst>(preheader->getTerminator());
  preheader_branch->setSuccessor(0, header);

  // Rewire the branch target of the body to the header.
  auto *body_branch = dyn_cast<llvm::BranchInst>(body->getTerminator());
  body_branch->setSuccessor(0, header);

  // Construct the preheader.
  builder.SetInsertPoint(preheader->getTerminator());

  // If the collection is associative, create an AssocKeysInst.
  bool collection_is_assoc = isa<AssocArrayType>(&collection_type);
  auto *iterable = &collection;
  if (collection_is_assoc) {
    // Create the AssocKeysInst
    auto &keys = MEMOIR_SANITIZE(builder.CreateKeysInst(&collection),
                                 "Failed to create KeysInst!");

    // Set the iterable collection to the AssocKeysInst.
    iterable = &keys.getCallInst();
  }

  // Create a call to get the size of the instruction.
  //   %n = size(%collection)
  auto *collection_size =
      &builder.CreateSizeInst(iterable, {}, "fold.size.")->getCallInst();

  // Get the index type.
  auto *index_type = cast<llvm::IntegerType>(collection_size->getType());
  auto index_bitwidth = index_type->getBitWidth();

  // Construct the header of the loop.
  builder.SetInsertPoint(header);
  //   %i = phi [0, preheader], [%i.inc, body]
  auto *index = builder.CreatePHI(index_type, 2, "index.");
  index->addIncoming(builder.getIntN(index_bitwidth, 0), preheader);
  //   %check = %i < %n
  auto *index_check =
      builder.CreateICmpEQ(index, collection_size, "index.check.");
  //   %next = %i + 1
  auto *index_next = builder.CreateAdd(index,
                                       builder.getIntN(index_bitwidth, 1),
                                       "index.next.");
  index->addIncoming(index_next, body);
  //   br %end, next, exit
  builder.CreateCondBr(index_check, exit, body);

  // Update the builder's insertion point to the index PHI.
  builder.SetInsertPoint(index);

  // Create a PHI node for the accumulator value.
  auto *accumulator_llvm_type = I.getCallInst().getType();
  auto &accumulator = MEMOIR_SANITIZE(
      builder.CreatePHI(accumulator_llvm_type, 2, "fold.accum.phi."),
      "Unable to create PHI node for accumulator!");

  // Update the builder's insertion point to the start of the loop.
  builder.SetInsertPoint(body->getFirstInsertionPt());

  // If this is a reverse fold, subtract the size of the iterable collection
  // from the index.
  llvm::Value *iterable_index = index;
  if (I.isReverse()) {
    iterable_index = builder.CreateSub(collection_size, index_next);
  }

  // If the collection is associative, read the key for this iteration.
  llvm::Value *key = iterable_index;
  if (collection_is_assoc) {
    // Fetch the key type.
    auto *assoc_type = cast<AssocArrayType>(&collection_type);
    auto &key_type = assoc_type->getKeyType();

    // Read the key from the keys sequence.
    auto &read_key = MEMOIR_SANITIZE(
        builder.CreateReadInst(key_type, iterable, { iterable_index }),
        "Failed to create ReadInst for AssocKeys!");

    // Save the key for later.
    key = &read_key.getValueRead();
  }

  // Insert a Get/ReadInst at the beginning of the loop.
  llvm::Value *element = nullptr;
  auto &element_type = collection_type.getElementType();
  if (isa<VoidType>(&element_type)) {
    // Do nothing.
  } else if (isa<TupleType>(&element_type)) {
    // Read the value from the collection.
    auto &read_value =
        MEMOIR_SANITIZE(builder.CreateGetInst(&collection, { key }),
                        "Failed to create IndexGetInst!");

    // Save the element read for later.
    element = &read_value.getCallInst();

  } else {
    // Read the value from the collection.
    auto &read_value = MEMOIR_SANITIZE(
        builder.CreateReadInst(element_type, &collection, { key }),
        "Failed to create ReadInst!");

    // Save the element read for later.
    element = &read_value.getCallInst();
  }

  return HeaderInfo(*preheader,
                    *header,
                    *body,
                    *exit,
                    accumulator,
                    *key,
                    element);
}

} // namespace detail

bool lower_fold(FoldInst &I,
                llvm::Value &collection,
                Type &type,
                llvm::Function *begin_func,
                llvm::Function *next_func,
                llvm::Type *iter_type,
                std::function<void(llvm::Value &, llvm::Value &)> coalesce,
                std::function<void(llvm::Instruction &)> cleanup) {

  bool destructing_ssa = (begin_func and next_func and iter_type);

  auto &collection_type = MEMOIR_SANITIZE(dyn_cast<CollectionType>(&type),
                                          "Lowering fold over non-collection!");

  // Above the fold instruction, create a SizeInst for the collection,
  MemOIRBuilder builder(I);

  // Insert the loop header.
  auto header_info =
      (destructing_ssa)
          ? detail::lower_fold_header(I,
                                      builder,
                                      collection,
                                      collection_type,
                                      *begin_func,
                                      *next_func,
                                      *iter_type)
          : detail::lower_fold_header(I, builder, collection, collection_type);

  // Unpack the header info.
  auto &loop_preheader = header_info.preheader;
  auto &loop_header = header_info.header;
  auto &loop_body = header_info.body;
  auto &loop_exit = header_info.exit;
  auto &accumulator = header_info.accumulator;
  auto &key = header_info.key;
  auto *element = header_info.element;

  // Fetch information about the fold body
  auto &body = I.getBody();
  auto *body_type = body.getFunctionType();

  // Construct the list of arguments to pass into the body.
  Vector<llvm::Value *> arguments = { &accumulator, &key };
  if (element) {
    arguments.push_back(element);
  }
  arguments.insert(arguments.end(), I.closed_begin(), I.closed_end());

  // Create a call to the fold body so that we can inline it.
  auto &call = MEMOIR_SANITIZE(
      builder.CreateCall(body_type, &body, arguments, "fold.accum."),
      "Failed to create call to the FoldInst body!");

  // For each predecessor of the loop header:
  //  - If it _is_ the the loop pre-header, set the incoming value of the
  //    accumulator to be the initial operand of the FoldInst.
  //  - If it is _not_ the loop pre-header, set the incoming value of the
  //    accumulator to be the result of the call.
  for (auto *pred : llvm::predecessors(&loop_header)) {
    auto *incoming = (pred == &loop_preheader) ? &I.getInitial() : &call;
    MEMOIR_ASSERT(accumulator.getType() == incoming->getType(),
                  "Mismatched PHI type for accumulator of ",
                  I);
    accumulator.addIncoming(incoming, pred);
  }

  // Replace all uses of the fold instruction result with the call result.
  coalesce(I.getCallInst(), accumulator);

  // For each closed argument:
  auto closed_idx = -1;
  for (auto *closed : I.closed()) {
    ++closed_idx;

    // Check that it is a collection.
    if (not Type::value_is_object(*closed)) {
      continue;
    }

    // See if it has a RetPHI for the original fold.
    RetPHIInst *found_ret_phi = nullptr;
    for (auto &closed_use : closed->uses()) {
      auto *closed_user = closed_use.getUser();

      if (auto *closed_user_as_inst =
              dyn_cast_or_null<llvm::Instruction>(closed_user)) {

        // If the user is a RetPHI:
        if (auto *ret_phi = into<RetPHIInst>(closed_user_as_inst)) {

          // Ensure that the RetPHI is for FoldInst.
          if (ret_phi->getCalledFunction()
              == I.getCallInst().getCalledFunction()) {

            if (not found_ret_phi) {
              found_ret_phi = ret_phi;
            } else {
              auto *fold_bb = I.getParent();
              auto *new_bb = ret_phi->getParent();
              auto *old_bb = found_ret_phi->getParent();
              if (fold_bb != old_bb and fold_bb == new_bb) {
                found_ret_phi = ret_phi;
              } else if (fold_bb != new_bb and fold_bb == old_bb) {
                continue;
              } else {
                println(*loop_header.getParent());
                println(*ret_phi);
                println(*found_ret_phi);
                MEMOIR_UNREACHABLE("Cannot disambiguate between two RetPHIs!"
                                   "Need to add a dominance check here.");
              }
            }
          } else {
            debugln("wrong function!");
          }
        }
      }
    }

    // If we could not find a RetPHI for the closed argument, then continue.
    if (not found_ret_phi) {
      warnln("Failed to find ret phi for closed operand");
      continue;
    }

    // If it does, we need to patch its DEF-USE chain.
    llvm::Value *closed_result;
    if (not destructing_ssa) {
      //  - Add a PHI for it in the loop.
      builder.SetInsertPoint(&accumulator);
      auto *closed_phi =
          builder.CreatePHI(closed->getType(), 2, "fold.closed.phi.");

      //  -- Update the use of the closed variabled to be the new PHI.
      auto &element_type = collection_type.getElementType();
      auto closed_offset = isa<VoidType>(&element_type) ? 2 : 3;
      call.setOperand(closed_idx + closed_offset, closed_phi);

      //  -- Insert a RetPHI after the call.
      builder.SetInsertPoint(call.getNextNode());
      auto *closed_ret_phi =
          builder.CreateRetPHI(closed_phi, &body, "fold.closed.");

      //  -- Wire up the PHI with the original value and the RetPHI.
      for (auto *pred : llvm::predecessors(closed_phi->getParent())) {
        auto *incoming =
            (pred == &loop_preheader) ? closed : &closed_ret_phi->getCallInst();

        closed_phi->addIncoming(incoming, pred);
      }

      closed_result = closed_phi;

    } else {
      closed_result = &found_ret_phi->getInput();
    }

    //  -- Replace uses of the original RetPHI with the continuation PHI.
    auto &ret_phi_inst = found_ret_phi->getCallInst();
    coalesce(ret_phi_inst, *closed_result);

    //  -- Delete the old RetPHI.
    cleanup(ret_phi_inst);
  }

  return true;

  // Now, try to inline the fold function.
  llvm::InlineFunctionInfo IFI;
  auto inline_result = llvm::memoir::InlineFunction(call, IFI);

  // If inlining failed, send a warning and continue.
  if (not inline_result.isSuccess()) {
    warnln("Inlining fold function failed.\n  Reason: ",
           inline_result.getFailureReason());
  }

  return true;
}

} // namespace llvm::memoir
