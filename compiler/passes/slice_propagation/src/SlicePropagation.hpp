#ifndef MEMOIR_SLICEPROPAGATION_H
#define MEMOIR_SLICEPROPAGATION_H
#pragma once

// LLVM
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"

#include "llvm/Pass.h"

#include "llvm/Analysis/CFG.h"
#include "llvm/IR/Dominators.h"

// NOELLE
#include "noelle/core/Noelle.hpp"

// MemOIR
#include "memoir/ir/Builder.hpp"
#include "memoir/ir/CollectionVisitor.hpp"
#include "memoir/ir/Function.hpp"
#include "memoir/ir/InstVisitor.hpp"
#include "memoir/ir/Instructions.hpp"

#include "memoir/analysis/CollectionAnalysis.hpp"
#include "memoir/analysis/StructAnalysis.hpp"
#include "memoir/analysis/TypeAnalysis.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/support/InternalDatatypes.hpp"
#include "memoir/support/Print.hpp"

#include "memoir/utility/FunctionNames.hpp"
#include "memoir/utility/Metadata.hpp"

/*
 * This file provides utility classes for the Slice Propagation pass.
 *
 * Author(s): Tommy McMichen
 * Created: March 27, 2023
 */

namespace llvm::memoir {

class SlicePropagation
  : protected llvm::memoir::CollectionVisitor<SlicePropagation, bool>,
    protected llvm::memoir::InstVisitor<SlicePropagation, llvm::Value *> {
  friend class llvm::memoir::CollectionVisitor<SlicePropagation, bool>;
  friend class llvm::memoir::InstVisitor<SlicePropagation, llvm::Value *>;
  friend class llvm::InstVisitor<SlicePropagation, llvm::Value *>;

public:
  /**
   * Construct a new SlicePropagation object.
   *
   * @param M A reference to an LLVM Module.
   * @param P A reference to the LLVM Pass being run.
   * @param noelle A reference to the NOELLE analysis.
   */
  SlicePropagation(llvm::Module &M,
                   llvm::Pass &P,
                   llvm::noelle::Noelle &noelle);

  /**
   * Analyze the given module for opportunities to propagate slices.
   *
   * @return True if analysis succeeded. False if analysis failed.
   // */
  bool analyze();

  /**
   * Transform the module by propagating slices.
   *
   * @return True if the module changed. False if no transformations were
   * performed.
   */
  bool transform();

  /**
   * Delete a SlicePropagation object.
   */
  ~SlicePropagation();

protected:
  struct SlicePropagationCandidate {
  public:
    SlicePropagationCandidate(llvm::Use &use,
                              llvm::Value &left_index,
                              llvm::Value &right_index,
                              SlicePropagationCandidate *parent)
      : use(use),
        left_index(left_index),
        right_index(right_index),
        parent(parent) {
      // Do nothing.
    }

    SlicePropagationCandidate(llvm::Use &use,
                              llvm::Value &left_index,
                              llvm::Value &right_index)
      : SlicePropagationCandidate(use, left_index, right_index, nullptr) {
      // Do nothing.
    }

    SlicePropagationCandidate(llvm::Use &use, SlicePropagationCandidate *parent)
      : SlicePropagationCandidate(use,
                                  parent->left_index,
                                  parent->right_index,
                                  parent) {
      // Do nothing.
    }

    ~SlicePropagationCandidate() {}

    // Borrowed state.
    SlicePropagationCandidate *parent;
    llvm::Use &use;

    // TODO: convert these into being llvm::ValueExpressions
    llvm::Value &left_index;
    llvm::Value &right_index;
  };

  // Owned state.
  set<SlicePropagationCandidate *> all_candidates;

  // Borrowed state.
  llvm::Module &M;
  llvm::Pass &P;
  llvm::noelle::Noelle &noelle;
  TypeAnalysis &TA;
  StructAnalysis &SA;
  CollectionAnalysis &CA;
  set<llvm::Value *> visited;

  set<SlicePropagationCandidate *> leaf_candidates;
  SlicePropagationCandidate *candidate;
  set<llvm::Value *> values_to_delete;

  // CollectionVisitor methods for analysis.
  bool visitBaseCollection(BaseCollection &C);
  bool visitFieldArray(FieldArray &C);
  bool visitNestedCollection(NestedCollection &C);
  bool visitReferencedCollection(ReferencedCollection &C);
  bool visitControlPHICollection(ControlPHICollection &C);
  bool visitRetPHICollection(RetPHICollection &C);
  bool visitArgPHICollection(ArgPHICollection &C);
  bool visitDefPHICollection(DefPHICollection &C);
  bool visitUsePHICollection(UsePHICollection &C);
  bool visitJoinPHICollection(JoinPHICollection &C);
  bool visitSliceCollection(SliceCollection &C);

  // Transformation.
  llvm::Value *handleCandidate(SlicePropagationCandidate &SPC);

  // InstVisitor methods for transformation.
  llvm::Value *visitArgument(llvm::Argument &A);
  llvm::Value *visitInstruction(llvm::Instruction &I);
  llvm::Value *visitLLVMCallInst(llvm::CallInst &I);
  llvm::Value *visitSequenceAllocInst(SequenceAllocInst &I);
  llvm::Value *visitJoinInst(JoinInst &I);
  llvm::Value *visitSliceInst(SliceInst &I);

  // Internal helpers.
  bool checkVisited(llvm::Value &V);
  void recordVisit(llvm::Value &V);
  void markForDeletion(llvm::Value &V);
};

} // namespace llvm::memoir
#endif
