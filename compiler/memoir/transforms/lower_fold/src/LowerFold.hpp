#ifndef MEMOIR_TRANSFORMS_LOWERFOLD_H
#define MEMOIR_TRANSFORMS_LOWERFOLD_H

// LLVM
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"

// MEMOIR
#include "memoir/ir/Instructions.hpp"

#include "memoir/support/Casting.hpp"
#include "memoir/support/DataTypes.hpp"

#include "memoir/lowering/LowerFold.hpp"

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

  bool transform() {
    // Collect all fold instructions to be lowered.
    Vector<FoldInst *> folds = {};
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
      // TODO, construct a get for the nested collection.
      if (lower_fold(*fold, fold->getObject(), fold->getElementType())) {
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
  Set<llvm::Instruction *> to_cleanup;
};

} // namespace llvm::memoir

#endif // MEMOIR_TRANSFORMS_LOWERFOLD_H
