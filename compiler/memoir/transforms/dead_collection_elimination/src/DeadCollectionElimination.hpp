#ifndef MEMOIR_DEADCOLLECTIONELIMINATION_H
#define MEMOIR_DEADCOLLECTIONELIMINATION_H
#pragma once

// LLVM
#include "llvm/IR/Dominators.h"

#include "llvm/Analysis/CFG.h"

// MemOIR
#include "memoir/ir/Builder.hpp"
#include "memoir/ir/InstVisitor.hpp"
#include "memoir/ir/Instructions.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/support/InternalDatatypes.hpp"
#include "memoir/support/Print.hpp"

#include "memoir/utility/FunctionNames.hpp"
#include "memoir/utility/Metadata.hpp"

/*
 * This class eliminates dead collections within an MemOIR program.
 *
 * Author(s): Tommy McMichen
 * Created: April 3, 2023
 */

namespace llvm::memoir {

class DeadCollectionElimination
  : InstVisitor<DeadCollectionElimination, set<llvm::Value *>> {
  friend class InstVisitor<DeadCollectionElimination, set<llvm::Value *>>;
  friend class llvm::InstVisitor<DeadCollectionElimination, set<llvm::Value *>>;

public:
  DeadCollectionElimination(llvm::Module &M) {
    // Run dead collection elimination.
    this->run(M);
  }

protected:
  // Top-level driver.
  bool run(llvm::Module &M) {
    return this->transform(this->analyze(M));
  }

  // Analysis
  set<llvm::Value *> analyze(llvm::Module &M) {
    // Find dead collections.
    set<llvm::Value *> dead_values;
    for (auto &F : M) {
      for (auto &BB : F) {
        for (auto &I : BB) {
          // Visit the instruction.
          auto discovered = this->visit(I);
          dead_values.insert(discovered.begin(), discovered.end());
        }
      }
    }

    return dead_values;
  }

  set<llvm::Value *> visitInstruction(llvm::Instruction &I) {
    // Do nothing.
    return {};
  }

  set<llvm::Value *> visitAllocInst(AllocInst &I) {
    set<llvm::Value *> dead_values = {};

    auto &llvm_inst = I.getCallInst();
    if (llvm_inst.hasNUses(0)) {
      dead_values.insert(&llvm_inst);
    }

    return dead_values;
  }

  set<llvm::Value *> visitInsertInst(InsertInst &I) {
    set<llvm::Value *> dead_values = {};

    auto &llvm_inst = I.getCallInst();
    if (llvm_inst.hasNUses(0)) {
      dead_values.insert(&llvm_inst);
    }

    return dead_values;
  }

  set<llvm::Value *> visitRemoveInst(RemoveInst &I) {
    set<llvm::Value *> dead_values = {};

    auto &llvm_inst = I.getCallInst();
    if (llvm_inst.hasNUses(0)) {
      dead_values.insert(&llvm_inst);
    }

    return dead_values;
  }

  set<llvm::Value *> visitCopyInst(CopyInst &I) {
    set<llvm::Value *> dead_values = {};

    auto &llvm_inst = I.getCallInst();
    if (llvm_inst.hasNUses(0)) {
      dead_values.insert(&llvm_inst);
    }

    return dead_values;
  }

  set<llvm::Value *> visitSwapInst(SwapInst &I) {
    set<llvm::Value *> dead_values = {};

    auto &llvm_inst = I.getCallInst();
    if (llvm_inst.hasNUses(0)) {
      dead_values.insert(&llvm_inst);
    }

    return dead_values;
  }

  set<llvm::Value *> visitDefineStructTypeInst(DefineStructTypeInst &I) {
    return {};
  }

  set<llvm::Value *> visitTypeInst(TypeInst &I) {
    set<llvm::Value *> dead_values = {};

    auto &llvm_inst = I.getCallInst();
    if (llvm_inst.hasNUses(0)) {
      dead_values.insert(&llvm_inst);
    }

    return dead_values;
  }

  set<llvm::Value *> visitAssertStructTypeInst(AssertStructTypeInst &I) {
    set<llvm::Value *> dead_values = {};

    auto &llvm_inst = I.getCallInst();
    dead_values.insert(&llvm_inst);

    auto &type_operand = I.getTypeOperand();
    if (type_operand.hasNUses(1)) {
      dead_values.insert(&type_operand);
    }

    return dead_values;
  }

  set<llvm::Value *> visitAssertCollectionTypeInst(
      AssertCollectionTypeInst &I) {
    set<llvm::Value *> dead_values = {};

    auto &llvm_inst = I.getCallInst();
    dead_values.insert(&llvm_inst);

    auto &type_operand = I.getTypeOperand();
    if (type_operand.hasNUses(1)) {
      dead_values.insert(&type_operand);
    }

    return dead_values;
  }

  set<llvm::Value *> visitReturnTypeInst(ReturnTypeInst &I) {
    set<llvm::Value *> dead_values = {};

    auto &llvm_inst = I.getCallInst();
    dead_values.insert(&llvm_inst);

    auto &type_operand = I.getTypeOperand();
    if (type_operand.hasNUses(1)) {
      dead_values.insert(&type_operand);
    }

    return dead_values;
  }

  // Transformation
  bool transform(set<llvm::Value *> &&dead_values) {
    // Drop all references to dead values that are Instructions.
    set<llvm::Value *> values_ready_to_delete = {};
    for (auto dead_value : dead_values) {
      // Sanity check.
      if (!dead_value) {
        continue;
      }

      // If this is an instruction, let's drop all references first.
      if (auto dead_inst = dyn_cast<llvm::Instruction>(dead_value)) {
        dead_inst->removeFromParent();
        dead_inst->dropAllReferences();
      }

      values_ready_to_delete.insert(dead_value);
    }

    // Delete all values.
    for (auto dead_value : values_ready_to_delete) {
      dead_value->deleteValue();
    }

    return true;
  }
};

} // namespace llvm::memoir

#endif
