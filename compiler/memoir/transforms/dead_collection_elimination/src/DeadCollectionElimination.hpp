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
#include "memoir/support/DataTypes.hpp"
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
  : InstVisitor<DeadCollectionElimination, Set<llvm::Value *>> {
  friend class InstVisitor<DeadCollectionElimination, Set<llvm::Value *>>;
  friend class llvm::InstVisitor<DeadCollectionElimination, Set<llvm::Value *>>;

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
  Set<llvm::Value *> analyze(llvm::Module &M) {
    // Find dead collections.
    Set<llvm::Value *> dead_values;
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

  Set<llvm::Value *> visitInstruction(llvm::Instruction &I) {
    // Do nothing.
    return {};
  }

  Set<llvm::Value *> visitAllocInst(AllocInst &I) {
    Set<llvm::Value *> dead_values = {};

    auto &llvm_inst = I.getCallInst();
    if (llvm_inst.hasNUses(0)) {
      dead_values.insert(&llvm_inst);
    }

    return dead_values;
  }

  Set<llvm::Value *> visitInsertInst(InsertInst &I) {
    Set<llvm::Value *> dead_values = {};

    auto &llvm_inst = I.getCallInst();
    if (llvm_inst.hasNUses(0)) {
      dead_values.insert(&llvm_inst);
    }

    return dead_values;
  }

  Set<llvm::Value *> visitRemoveInst(RemoveInst &I) {
    Set<llvm::Value *> dead_values = {};

    auto &llvm_inst = I.getCallInst();
    if (llvm_inst.hasNUses(0)) {
      dead_values.insert(&llvm_inst);
    }

    return dead_values;
  }

  Set<llvm::Value *> visitCopyInst(CopyInst &I) {
    Set<llvm::Value *> dead_values = {};

    auto &llvm_inst = I.getCallInst();
    if (llvm_inst.hasNUses(0)) {
      dead_values.insert(&llvm_inst);
    }

    return dead_values;
  }

  Set<llvm::Value *> visitSwapInst(SwapInst &I) {
    Set<llvm::Value *> dead_values = {};

    auto &llvm_inst = I.getCallInst();
    if (llvm_inst.hasNUses(0)) {
      dead_values.insert(&llvm_inst);
    }

    return dead_values;
  }

  Set<llvm::Value *> visitDefineTupleTypeInst(DefineTupleTypeInst &I) {
    return {};
  }

  Set<llvm::Value *> visitTypeInst(TypeInst &I) {
    Set<llvm::Value *> dead_values = {};

    auto &llvm_inst = I.getCallInst();
    if (llvm_inst.hasNUses(0)) {
      dead_values.insert(&llvm_inst);
    }

    return dead_values;
  }

  Set<llvm::Value *> visitAssertTupleTypeInst(AssertTupleTypeInst &I) {
    Set<llvm::Value *> dead_values = {};

    auto &llvm_inst = I.getCallInst();
    dead_values.insert(&llvm_inst);

    auto &type_operand = I.getTypeOperand();
    if (type_operand.hasNUses(1)) {
      dead_values.insert(&type_operand);
    }

    return dead_values;
  }

  Set<llvm::Value *> visitAssertCollectionTypeInst(
      AssertCollectionTypeInst &I) {
    Set<llvm::Value *> dead_values = {};

    auto &llvm_inst = I.getCallInst();
    dead_values.insert(&llvm_inst);

    auto &type_operand = I.getTypeOperand();
    if (type_operand.hasNUses(1)) {
      dead_values.insert(&type_operand);
    }

    return dead_values;
  }

  Set<llvm::Value *> visitReturnTypeInst(ReturnTypeInst &I) {
    Set<llvm::Value *> dead_values = {};

    auto &llvm_inst = I.getCallInst();
    dead_values.insert(&llvm_inst);

    auto &type_operand = I.getTypeOperand();
    if (type_operand.hasNUses(1)) {
      dead_values.insert(&type_operand);
    }

    return dead_values;
  }

  // Transformation
  bool transform(Set<llvm::Value *> &&dead_values) {
    // Drop all references to dead values that are Instructions.
    Set<llvm::Value *> values_ready_to_delete = {};
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
