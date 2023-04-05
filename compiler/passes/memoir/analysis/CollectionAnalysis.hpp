#ifndef MEMOIR_COLLECTIONANALYSIS_H
#define MEMOIR_COLLECTIONANALYSIS_H
#pragma once

#include <iostream>

#include "memoir/support/Assert.hpp"
#include "memoir/support/InternalDatatypes.hpp"

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"

#include "noelle/core/Noelle.hpp"

#include "memoir/ir/InstVisitor.hpp"

#include "memoir/ir/Collections.hpp"
#include "memoir/ir/Instructions.hpp"

/*
 * This file provides a simple analysis interface to query information
 *   about MemOIR collections in a program.
 *
 * Author(s): Tommy McMichen
 * Created: November 14, 2022
 */

namespace llvm::memoir {

class CollectionAnalysis
  : public llvm::memoir::InstVisitor<CollectionAnalysis, Collection *> {
  friend class llvm::memoir::InstVisitor<CollectionAnalysis, Collection *>;
  friend class llvm::InstVisitor<CollectionAnalysis, Collection *>;

public:
  /**
   * Get the singleton CollectionAnalysis instance.
   *
   * @return The CollectionAnalysis as a reference.
   */
  static CollectionAnalysis &get(Noelle &noelle);
  static CollectionAnalysis &get();

  /**
   * Get the Collection for a given llvm Use.
   *
   * @param U A reference to an llvm Use
   * @return The collection, or NULL if the Use is not a memoir collection.
   */
  static Collection *analyze(llvm::Use &U);

  /**
   * Get the Collection for a given llvm Value.
   *
   * @param V A reference to an llvm Value.
   * @return The collection, or NULL if the Value is not a memoir collection.
   */
  static Collection *analyze(llvm::Value &V);

  /**
   * Invalidate the singleton CollectionAnalysis instance.
   * This will clear all memoized analysis information.
   * This will also delete all owned state and release all borrowed state.
   */
  static void invalidate();

  /**
   * Get the Collection for a given llvm Use.
   *
   * @param use A reference to an llvm Use.
   * @return The collection, or NULL if the Use is not a memoir collection.
   */
  Collection *getCollection(llvm::Use &use);

  /**
   * Get the Collection for a given llvm Value.
   *
   * @param value A reference to an llvm Value.
   * @return The collection, or NULL if the Value is not a memoir collection.
   */
  Collection *getCollection(llvm::Value &value);

private:
  /*
   * Owned state
   */
  map<llvm::Value *, Collection *> value_to_collection;
  map<llvm::Use *, Collection *> use_to_collection;
  map<llvm::Function *, llvm::noelle::DominatorSummary *>
      function_to_dominator_summary;
  map<llvm::PHINode *, unsigned> current_phi_index;

  /*
   * Borrowed state
   */
  llvm::noelle::Noelle &noelle;

  /*
   * Internal helper functions
   */
  Collection *findExisting(llvm::Value &V);
  Collection *findExisting(MemOIRInst &I);
  Collection *findExisting(llvm::Use &U);
  void memoize(llvm::Value &V, Collection *C);
  void memoize(MemOIRInst &I, Collection *C);
  void memoize(llvm::Use &U, Collection *C);
  llvm::noelle::DominatorSummary &getDominators(llvm::Function &F);

  /*
   * Visitor methods
   */
  Collection *visitInstruction(llvm::Instruction &I);
  Collection *visitStructAllocInst(StructAllocInst &I);
  Collection *visitTensorAllocInst(TensorAllocInst &I);
  Collection *visitSequenceAllocInst(SequenceAllocInst &I);
  Collection *visitAssocArrayAllocInst(AssocArrayAllocInst &I);
  Collection *visitStructGetInst(StructGetInst &I);
  Collection *visitGetInst(GetInst &I);
  Collection *visitReadInst(ReadInst &I);
  Collection *visitJoinInst(JoinInst &I);
  Collection *visitSliceInst(SliceInst &I);
  Collection *visitPHINode(llvm::PHINode &I);
  Collection *visitLLVMCallInst(llvm::CallInst &I);
  Collection *visitReturnInst(llvm::ReturnInst &I);
  Collection *visitArgument(llvm::Argument &A);

  /*
   * Pass options
   */
  bool enable_use_phi;
  bool enable_def_phi;

  /*
   * Private constructor and logistics.
   */
  CollectionAnalysis(llvm::noelle::Noelle &noelle,
                     bool enable_def_phi,
                     bool enable_use_phi);

  CollectionAnalysis(llvm::noelle::Noelle &noelle);

  void _invalidate();

  static CollectionAnalysis *CA;
};

} // namespace llvm::memoir

#endif // MEMOIR_COLLECTIONANALYSIS_H
