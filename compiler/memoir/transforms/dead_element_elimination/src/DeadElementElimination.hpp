#ifndef MEMOIR_DEADCOLLECTIONELIMINATION_H
#define MEMOIR_DEADCOLLECTIONELIMINATION_H
#pragma once

// LLVM
#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "llvm/IR/Dominators.h"

#include "llvm/Analysis/CFG.h"

// NOELLE
#include "noelle/core/Noelle.hpp"

// MemOIR
#include "memoir/ir/Builder.hpp"
#include "memoir/ir/InstVisitor.hpp"
#include "memoir/ir/Instructions.hpp"

#include "memoir/analysis/LiveRangeAnalysis.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/support/InternalDatatypes.hpp"
#include "memoir/support/Print.hpp"

#include "memoir/utility/FunctionNames.hpp"
#include "memoir/utility/Metadata.hpp"

/*
 * This class eliminates dead element updates within an MemOIR program.
 *
 * Author(s): Tommy McMichen
 * Created: January 4, 2024
 */

namespace llvm::memoir {

class DeadElementElimination {
public:
  /**
   * Performs Dead Element Elimination on the input program @M.
   * Requires the results of Live Range Analysis @LRA.
   */
  DeadElementElimination(llvm::Module &M, LiveRangeAnalysis &LRA)
    : M(M),
      LRA(LRA) {
    // Run dead element elimination.
    this->_transformed = this->run();
  }

  /**
   * Queries wether the transformation modified the program or not.
   */
  bool transformed() const {
    return this->_transformed;
  }

protected:
  // Top-level driver.
  bool run() {
    // Acquire the results of the live range analyses.
    const auto &live_ranges = this->LRA.results();

    // For each live range result:
    for (auto const [collection, context_to_live_range] : live_ranges) {
      for (auto const [context, live_range] : context_to_live_range) {

        // If the context is defined, create a version of it unless it already
        // exists.
        if (context != nullptr) {
          MEMOIR_UNREACHABLE(
              "Context-sensitive dead-element elimination is unimplemented!");
        }

        // Get the definition instruction, if it exists.
        auto *inst = dyn_cast<llvm::Instruction>(collection);
        if (!inst) {
          continue;
        }

        debugln("DEE: ", *inst);
        debugln("     ", *live_range);

        // Ignore upper/lower range if they are max/min.
        auto &upper = live_range->get_upper();
        bool ignore_upper = isa<EndExpression>(&upper);

        auto &lower = live_range->get_lower();
        bool ignore_lower = false;
        if (auto *lower_const = dyn_cast<ConstantExpression>(&lower)) {
          if (auto *lower_const_int =
                  dyn_cast<llvm::ConstantInt>(&lower_const->getConstant())) {
            if (lower_const_int->getZExtValue() == 0) {
              ignore_lower = true;
            }
          }
        }

        // If we are ignoring upper and lower, ignore the range.
        if (ignore_upper && ignore_lower) {
          debugln("     ignoring range");
          continue;
        }

        // If the upper and lower value expressions are available:
        if ((ignore_upper || upper.isAvailable(*inst))
            && (ignore_lower || lower.isAvailable(*inst))) {
          // Materialize the upper and lower value expressions.
          llvm::Value *materialized_upper = nullptr;
          if (!ignore_upper) {
            materialized_upper = upper.materialize(*inst);
            if (!materialized_upper) {
              continue;
            }
          }
          llvm::Value *materialized_lower = nullptr;
          if (!ignore_lower) {
            materialized_lower = lower.materialize(*inst);
            if (!materialized_lower) {
              continue;
            }
          }

          // Perform the rewrite rule.
          if (auto *write_inst = into<IndexWriteInst>(inst)) {

            // Fetch the index of the write instruction.
            auto &index = write_inst->getIndexOfDimension(0);

            // We will first construct the conditional check on the index.
            MemOIRBuilder builder(inst);
            llvm::Value *cond = nullptr;
            if (materialized_lower) {
              auto *lower_cmp =
                  builder.CreateICmpULE(materialized_lower, &index);
              cond = lower_cmp;
            }
            if (materialized_upper) {
              auto *upper_cmp =
                  builder.CreateICmpULT(&index, materialized_upper);
              if (cond) {
                cond = builder.CreateAnd(upper_cmp, cond);
              } else {
                cond = upper_cmp;
              }
            }

            if (!cond) {
              MEMOIR_UNREACHABLE("Could not create the comparison!");
            }

            // Save the original basic block.
            auto *original_bb = inst->getParent();

            // Then, construct a hammock to move the write to.
            // NOTE: @new_terminator is the terminator of the "then" basic
            // block.
            auto *then_terminator =
                llvm::SplitBlockAndInsertIfThen(/* Condition = */ cond,
                                                /* Split Before = */ inst,
                                                /* Unreachable = */ false);

            // Create a PHI at the split point.
            builder.SetInsertPoint(inst);
            auto *phi = builder.CreatePHI(inst->getType(), 2, "dee");

            // Move the instruction to the "then" basic block.
            inst->moveBefore(then_terminator);

            // Replace uses of the write result with the PHI node.
            inst->replaceAllUsesWith(phi);

            // Patch up the PHI for the "then" branch.
            auto *then_bb = then_terminator->getParent();
            phi->addIncoming(inst, then_bb);

            // Patch up the PHI for the "else" branch.
            auto &original_collection = write_inst->getObjectOperand();
            phi->addIncoming(&original_collection, original_bb);

          } else if (auto *insert_inst = into<SeqInsertInst>(inst)) {

            // Fetch the insertion point.
            auto &index = insert_inst->getInsertionPoint();

            // We will first construct the conditional check on the index.
            MemOIRBuilder builder(inst);
            llvm::Value *cond = nullptr;
            if (materialized_upper) {
              auto *upper_cmp =
                  builder.CreateICmpULT(&index, materialized_upper);
              cond = upper_cmp;
            }

            if (!cond) {
              MEMOIR_UNREACHABLE("Could not create the comparison!");
            }

            // Save the original basic block.
            auto *original_bb = inst->getParent();

            // Then, construct a hammock to move the insert to.
            // NOTE: @new_terminator is the terminator of the "then" basic
            // block.
            auto *then_terminator =
                llvm::SplitBlockAndInsertIfThen(/* Condition = */ cond,
                                                /* Split Before = */ inst,
                                                /* Unreachable = */ false);

            // Create a PHI at the split point.
            builder.SetInsertPoint(inst);
            auto *phi = builder.CreatePHI(inst->getType(), 2, "dee");

            // Move the instruction to the "then" basic block.
            inst->moveBefore(then_terminator);

            // Replace uses of the insert result with the PHI node.
            inst->replaceAllUsesWith(phi);

            // Patch up the PHI for the "then" branch.
            auto *then_bb = then_terminator->getParent();
            phi->addIncoming(inst, then_bb);

            // Patch up the PHI for the "else" branch.
            auto &original_collection = insert_inst->getBaseCollection();
            phi->addIncoming(&original_collection, original_bb);

          } else if (auto *swap_inst = into<SeqSwapInst>(inst)) {
            warnln("Swap instruction is unimplemented!");
          } else if (auto *swap_within_inst = into<SeqSwapWithinInst>(inst)) {
            warnln("Swap instruction is unimplemented!");
          }
        }
      }
    }

    return true;
  }

  // Owned state.
  bool _transformed;

  // Borrowed state.
  llvm::Module &M;
  LiveRangeAnalysis &LRA;
};

} // namespace llvm::memoir

#endif
