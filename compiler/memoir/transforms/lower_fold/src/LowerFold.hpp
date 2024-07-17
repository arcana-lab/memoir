#ifndef MEMOIR_TRANSFORMS_LOWERFOLD_H
#define MEMOIR_TRANSFORMS_LOWERFOLD_H

// LLVM
#include "llvm/IR/CFG.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"

#include "llvm/Transforms/Utils/BasicBlockUtils.h"

// MEMOIR
#include "memoir/ir/Builder.hpp"
#include "memoir/ir/Instructions.hpp"
#include "memoir/transforms/utilities/Inlining.hpp"

#include "memoir/support/InternalDatatypes.hpp"

/*
 * Pass to lower fold instructions to a canonical loop form.
 *
 * Author: Tommy McMichen
 * Created: July 9, 2024
 */

namespace llvm::memoir {

class LowerFold {

public:
  LowerFold(llvm::Module &M) : M(M) {
    // Transform all fold operations.
    this->transformed |= this->transform();

    // Cleanup newly dead instructions.
    this->transformed |= this->cleanup();
  }

  static bool lower_fold(FoldInst &I) {
    // Split the instruction out of its basic block, such that:
    auto *header = I.getParent();

    auto *preheader =
        llvm::SplitBlock(header,
                         &I.getCallInst(),
                         /* DomTreeUpdater = */ (llvm::DomTreeUpdater *)nullptr,
                         /* LoopInfo = */ nullptr,
                         /* MemorySSAUpdater = */ nullptr,
                         /* BBName = */ "fold.loop.preheader.",
                         /* Before = */ true);

    auto *continuation =
        llvm::SplitBlock(header,
                         I.getCallInst().getNextNode(),
                         /* DomTreeUpdater = */ (llvm::DomTreeUpdater *)nullptr,
                         /* LoopInfo = */ nullptr,
                         /* MemorySSAUpdater = */ nullptr,
                         /* BBName = */ "fold.continuation.",
                         /* Before = */ false);

    // Fetch information about the collection being folded over.
    auto &collection = I.getCollection();
    auto &collection_type =
        MEMOIR_SANITIZE(dyn_cast_or_null<CollectionType>(type_of(collection)),
                        "FoldInst accessing non-collection type!");
    bool collection_is_assoc = isa<AssocArrayType>(&collection_type);

    // Above the fold instruction, create a SizeInst for the collection,
    MemOIRBuilder builder(I);

    auto *collection_size =
        &builder.CreateSizeInst(&collection, "fold.size.")->getCallInst();

    // Create a compare instruction to check if the collection is empty.
    auto *collection_nonempty = builder.CreateICmpNE(collection_size,
                                                     builder.getInt64(0),
                                                     "fold.nonempty.");

    // Split the header after the fold, creating an if-else based on the
    // comparison. The block will be inserted in the else side of the branch.
    auto *then_terminator =
        llvm::SplitBlockAndInsertIfThen(/* Cond = */ collection_nonempty,
                                        /* SplitBefore = */ &I.getCallInst(),
                                        /* Unreachable = */ false);

    // Insert a PHI before the FoldInst for the accumulator.
    builder.SetInsertPoint(&I.getCallInst());
    auto &continuation_phi =
        MEMOIR_SANITIZE(builder.CreatePHI(I.getCallInst().getType(), 2),
                        "Failed to create PHINode");

    // Update the incoming value from the non-then branch.
    auto *nonempty_block = collection_size->getParent();
    continuation_phi.addIncoming(&I.getInitial(), nonempty_block);

    // Replace all uses of the fold instruction result with the call result.
    I.getCallInst().replaceAllUsesWith(&continuation_phi);

    // If the collection is associative, create an AssocKeysInst.
    auto *iterable = &collection;
    if (collection_is_assoc) {
      // Make sure the builder's insertion point is correct.
      builder.SetInsertPoint(then_terminator);

      // Create the AssocKeysInst
      auto &keys = MEMOIR_SANITIZE(builder.CreateAssocKeysInst(&collection),
                                   "Failed to create AssocKeysInst!");

      // Set the iterable collection to the AssocKeysInst.
      iterable = &keys.getCallInst();
    }

    // Split the block before the fold instruction, creating a for-loop from 0
    // to collection_size.
    auto [loop_insertion_point, index] =
        llvm::SplitBlockAndInsertSimpleForLoop(collection_size,
                                               then_terminator);

    // Update the builder's insertion point to the index PHI.
    auto &index_phi =
        MEMOIR_SANITIZE(dyn_cast_or_null<llvm::PHINode>(index),
                        "Index of for loop is not a PHI, LLVM Lied!");
    builder.SetInsertPoint(&index_phi);

    // Create a PHI node for the accumulator value.
    auto *accumulator_llvm_type = I.getCallInst().getType();
    auto &accumulator = MEMOIR_SANITIZE(
        builder.CreatePHI(accumulator_llvm_type, 2, "fold.accum.phi."),
        "Unable to create PHI node for accumulator!");

    // Update the builder's insertion point to the start of the loop.
    builder.SetInsertPoint(loop_insertion_point);

    // If this is a reverse fold, subtract the size of the iterable collection
    // from the index.
    auto *iterable_index = index;
    if (I.isReverse()) {
      iterable_index = builder.CreateSub(collection_size, index);
      iterable_index = builder.CreateSub(
          iterable_index,
          llvm::ConstantInt::get(collection_size->getType(), 1));
    }

    // If the collection is associative, read the key for this iteration.
    llvm::Value *key = iterable_index;
    if (collection_is_assoc) {
      // Fetch the key type.
      auto *assoc_type = cast<AssocArrayType>(&collection_type);
      auto &key_type = assoc_type->getKeyType();

      // Read the key from the keys sequence.
      auto &read_key = MEMOIR_SANITIZE(
          builder.CreateIndexReadInst(key_type, iterable, iterable_index),
          "Failed to create IndexReadInst for AssocKeys!");

      // Save the key for later.
      key = &read_key.getValueRead();
    }

    // Insert a Get/ReadInst at the beginning of the loop.
    llvm::Value *element = nullptr;
    auto &element_type = collection_type.getElementType();
    if (isa<VoidType>(&element_type)) {
      // Do nothing.
    } else if (isa<StructType>(&element_type)) {
      if (collection_is_assoc) {
        // Read the value from the collection.
        auto &read_value = MEMOIR_SANITIZE(
            builder.CreateAssocGetInst(element_type, &collection, key),
            "Failed to create AssocGetInst!");

        // Save the element read for later.
        element = &read_value.getCallInst();

      } else {
        // Read the value from the collection.
        auto &read_value = MEMOIR_SANITIZE(
            builder.CreateIndexGetInst(element_type, &collection, key),
            "Failed to create IndexGetInst!");

        // Save the element read for later.
        element = &read_value.getCallInst();
      }
    } else {
      if (collection_is_assoc) {
        // Read the value from the collection.
        auto &read_value = MEMOIR_SANITIZE(
            builder.CreateAssocReadInst(element_type, &collection, key),
            "Failed to create AssocReadInst!");

        // Save the element read for later.
        element = &read_value.getCallInst();

      } else {
        // Read the value from the collection.
        auto &read_value = MEMOIR_SANITIZE(
            builder.CreateIndexReadInst(element_type, &collection, key),
            "Failed to create IndexReadInst!");

        // Save the element read for later.
        element = &read_value.getCallInst();
      }
    }

    // Fetch information about the fold function
    auto &function = I.getFunction();
    auto *function_type = function.getFunctionType();

    // Construct the list of arguments to pass into the function.
    vector<llvm::Value *> arguments = { &accumulator, key };
    if (element != nullptr) {
      arguments.push_back(element);
    }
    for (unsigned closed_idx = 0; closed_idx < I.getNumberOfClosed();
         ++closed_idx) {
      // Get the closed variable.
      auto &closed = I.getClosed(closed_idx);

      // Add the closed variable to the list of arguments.
      arguments.push_back(&closed);
    }

    // Create a call to the fold function so that we can inline it.
    auto &call = MEMOIR_SANITIZE(
        builder.CreateCall(function_type, &function, arguments, "fold.accum."),
        "Failed to create call to the FoldInst function!");

    // Wire the returned accumulator to its output PHI.
    for (auto *pred : llvm::predecessors(continuation_phi.getParent())) {
      if (pred != nonempty_block) {
        continuation_phi.addIncoming(&call, pred);
      }
    }

    // Split the call into its own basic block.
    auto *loop_body = call.getParent();

    llvm::SplitBlock(loop_body,
                     &call,
                     /* DomTreeUpdater = */ (llvm::DomTreeUpdater *)nullptr,
                     /* LoopInfo = */ nullptr,
                     /* MemorySSAUpdater = */ nullptr,
                     /* BBName = */ "fold.loop.header.",
                     /* Before = */ true);

    llvm::SplitBlock(loop_body,
                     call.getNextNode(),
                     /* DomTreeUpdater = */ (llvm::DomTreeUpdater *)nullptr,
                     /* LoopInfo = */ nullptr,
                     /* MemorySSAUpdater = */ nullptr,
                     /* BBName = */ "fold.loop.check.",
                     /* Before = */ false);

    loop_body->setName("fold.loop.body.");

    // Find the loop pre-header by finding which of the incoming values for
    // the index PHI is a constant zero.
    llvm::BasicBlock *loop_preheader = nullptr;
    auto *loop_header = index_phi.getParent();
    for (auto *pred : llvm::predecessors(loop_header)) {
      auto *index_incoming = index_phi.getIncomingValueForBlock(pred);

      // If the incoming index value is a constant, we found our pre-header.
      if (isa<llvm::ConstantInt>(index_incoming)) {
        loop_preheader = pred;
        break;
      }
    }
    MEMOIR_NULL_CHECK(loop_preheader,
                      "Unable to find preheader of the loop we created!");

    // For each closed argument:
    for (unsigned closed_idx = 0; closed_idx < I.getNumberOfClosed();
         ++closed_idx) {
      auto &closed = I.getClosed(closed_idx);

      // Check that it is a collection.
      if (not Type::value_is_collection_type(closed)) {
        continue;
      }

      // See if it has a RetPHI for the original fold.
      RetPHIInst *found_ret_phi = nullptr;
      for (auto &closed_use : closed.uses()) {
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
                MEMOIR_UNREACHABLE("Cannot disambiguate between two RetPHIs!"
                                   "Need to add a dominance check here.");
              }
            } else {
              println("wrong function!");
            }
          }
        }
      }

      // If we could not find a RetPHI for the closed argument, then continue.
      if (not found_ret_phi) {
        continue;
      }

      // If it does, we need to patch its DEF-USE chain.

      //  - Add a PHI for it in the loop.
      builder.SetInsertPoint(&accumulator);
      auto *closed_phi =
          builder.CreatePHI(closed.getType(), 2, "fold.closed.phi.");

      //  -- Update the use of the closed variabled to be the new PHI.
      auto closed_offset = isa<VoidType>(&element_type) ? 2 : 3;
      call.setOperand(closed_idx + closed_offset, closed_phi);

      //  -- Insert a RetPHI after the call.
      builder.SetInsertPoint(call.getNextNode());
      auto *closed_ret_phi =
          builder.CreateRetPHI(closed_phi, &function, "fold.closed.");

      //  -- Wire up the PHI with the original value and the RetPHI.
      for (auto *pred : llvm::predecessors(loop_header)) {
        if (pred == loop_preheader) {
          closed_phi->addIncoming(&closed, loop_preheader);
        } else {
          closed_phi->addIncoming(&closed_ret_phi->getCallInst(), pred);
        }
      }

      //  - Add a PHI for it in the loop's continuation.
      builder.SetInsertPoint(&continuation_phi);
      auto *closed_continuation_phi =
          builder.CreatePHI(closed.getType(),
                            2,
                            "fold.continuation.closed.phi.");

      //  -- Wire up the continuation PHI with the original value and RetPHI.
      for (auto *pred :
           llvm::predecessors(closed_continuation_phi->getParent())) {
        if (pred == nonempty_block) {
          closed_continuation_phi->addIncoming(&closed, nonempty_block);
        } else {
          closed_continuation_phi->addIncoming(&closed_ret_phi->getCallInst(),
                                               pred);
        }
      }

      //  -- Replace uses of the original RetPHI with the continuation PHI.
      found_ret_phi->getCallInst().replaceAllUsesWith(closed_continuation_phi);

      //  -- Delete the old RetPHI.
      found_ret_phi->getCallInst().eraseFromParent();
    }

    // For each predecessor of the loop header:
    //  - If it _is_ the the loop pre-header, set the incoming value of the
    //    accumulator to be the initial operand of the FoldInst.
    //  - If it is _not_ the loop pre-header, set the incoming value of the
    //    accumulator to be the result of the call.
    for (auto *pred : llvm::predecessors(loop_header)) {
      if (pred == loop_preheader) {
        accumulator.addIncoming(&I.getInitial(), loop_preheader);
      } else {
        accumulator.addIncoming(&call, pred);
      }
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

  bool transform() {
    // Collect all fold instructions to be lowered.
    vector<FoldInst *> folds = {};
    for (auto &F : M) {
      for (auto &BB : F) {
        for (auto &I : BB) {
          if (auto *fold = into<FoldInst>(I)) {
            folds.push_back(fold);
          }
        }
      }
    }

    // If there are no folds, return false.
    if (folds.size() == 0) {
      return false;
    }

    // Lower each fold.
    for (auto *fold : folds) {
      if (lower_fold(*fold)) {
        this->to_cleanup.insert(&fold->getCallInst());
      }
    }

    // If we got this far, we modified the code. Return true.
    return true;
  }

  bool cleanup() {
    if (this->to_cleanup.empty()) {
      return false;
    }

    // TODO: erase instructions from parent
    for (auto *inst : this->to_cleanup) {
      inst->eraseFromParent();
    }

    return true;
  }

private:
  // Owned state.
  bool transformed;

  // Borrowed state.
  llvm::Module &M;
  set<llvm::Instruction *> to_cleanup;
};

} // namespace llvm::memoir

#endif // MEMOIR_TRANSFORMS_LOWERFOLD_H
