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
    SlicePropagationCandidate(SliceInst &slice) : slice(slice) {
      // Do nothing.
    }

    ~SlicePropagationCandidate() {}

    // Borrowed state.
    SliceInst &slice;
    SlicePropagationCandidate *parent;
  };

  // Owned state.
  map<SliceInst *, set<SlicePropagationCandidate *>> candidates;

  // Borrowed state.
  llvm::Module &M;
  llvm::Pass &P;
  llvm::noelle::Noelle &noelle;
  TypeAnalysis &TA;
  StructAnalysis &SA;
  CollectionAnalysis &CA;

  map<llvm::Value *, pair<llvm::Value *, llvm::Value *>> collections_to_slice;
  set<llvm::Value *> visited;
  SliceInst *slice_under_test;

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

  // InstVisitor methods for transformation.
  llvm::Value *visitArgument(llvm::Argument &A);
  llvm::Value *visitInstruction(llvm::Instruction &I);
  llvm::Value *visitSequenceAllocInst(SequenceAllocInst &I);
  llvm::Value *visitJoinInst(JoinInst &I);
  llvm::Value *visitSliceInst(SliceInst &I);

  // Internal helpers.
  bool checkVisited(llvm::Value &V);
  void recordVisit(llvm::Value &V);
};

} // namespace llvm::memoir
#endif
