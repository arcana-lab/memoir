#ifndef MEMOIR_SIZEANALYSIS_H
#define MEMOIR_SIZEANALYSIS_H
#pragma once

// LLVM

// NOELLE
#include "noelle/core/Noelle.hpp"

// MemOIR
#include "memoir/ir/InstVisitor.hpp"

#include "memoir/support/InternalDatatypes.hpp"

#include "memoir/analysis/ValueNumbering.hpp"

/*
 * This file provides a simple analysis to determine, symbolically or
 * statically, the size of a collection at a given program point.
 *
 * Author(s): Tommy McMichen
 * Created: April 11, 2023
 */

namespace llvm::memoir {
class SizeAnalysis : InstVisitor<SizeAnalysis, ValueExpression *> {
  friend class InstVisitor<SizeAnalysis, ValueExpression *>;
  friend class llvm::InstVisitor<SizeAnalysis, ValueExpression *>;

public:
  /**
   * Creates a new Size Analysis.
   * @param noelle A reference to Noelle
   * @param VN A reference to a ValueNumbering analysis
   */
  SizeAnalysis(arcana::noelle::Noelle &noelle, ValueNumbering &VN)
    : noelle(noelle),
      VN(VN) {}
  ~SizeAnalysis();

  /**
   * Get's an expression summarizing the size of a given collection.
   *
   * @param C A reference to a MemOIR collection.
   * @returns A ValueExpression representing the size of the collection, or NULL
   *          if the size could not be determined.
   */
  ValueExpression *getSize(llvm::Value &C);

protected:
  // Analysis visitor methods.
  // TODO: flesh me out!
  ValueExpression *visitArgument(llvm::Argument &A);
  ValueExpression *visitInstruction(llvm::Instruction &I);
  ValueExpression *visitSequenceAllocInst(SequenceAllocInst &I);
  ValueExpression *visitPHINode(llvm::PHINode &I);
  ValueExpression *visitRetPHIInst(RetPHIInst &I);
  ValueExpression *visitArgPHIInst(ArgPHIInst &I);
  ValueExpression *visitDefPHIInst(DefPHIInst &I);
  ValueExpression *visitUsePHIInst(UsePHIInst &I);

  // Owned state.

  // Borrowed state.
  arcana::noelle::Noelle &noelle;
  ValueNumbering &VN;
};

} // namespace llvm::memoir

#endif
