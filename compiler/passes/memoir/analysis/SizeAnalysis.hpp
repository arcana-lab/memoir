#ifndef MEMOIR_SIZEANALYSIS_H
#define MEMOIR_SIZEANALYSIS_H
#pragma once

// LLVM

// NOELLE
#include "noelle/core/Noelle.hpp"

// MemOIR
#include "memoir/ir/CollectionVisitor.hpp"

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
class SizeAnalysis : CollectionVisitor<SizeAnalysis, ValueExpression *> {
  friend class CollectionVisitor<SizeAnalysis, ValueExpression *>;

public:
  /**
   * Creates a new Size Analysis.
   * @param noelle A reference to Noelle
   * @param VN A reference to a ValueNumbering analysis
   */
  SizeAnalysis(llvm::noelle::Noelle &noelle, ValueNumbering &VN)
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
  ValueExpression *getSize(Collection &C);

protected:
  // Analysis visitor methods.
  ValueExpression *visitBaseCollection(BaseCollection &C);
  ValueExpression *visitFieldArray(FieldArray &C);
  ValueExpression *visitNestedCollection(NestedCollection &C);
  ValueExpression *visitReferencedCollection(ReferencedCollection &C);
  ValueExpression *visitControlPHICollection(ControlPHICollection &C);
  ValueExpression *visitRetPHICollection(RetPHICollection &C);
  ValueExpression *visitArgPHICollection(ArgPHICollection &C);
  ValueExpression *visitDefPHICollection(DefPHICollection &C);
  ValueExpression *visitUsePHICollection(UsePHICollection &C);
  ValueExpression *visitJoinPHICollection(JoinPHICollection &C);
  ValueExpression *visitSliceCollection(SliceCollection &C);

  // Owned state.

  // Borrowed state.
  llvm::noelle::Noelle &noelle;
  ValueNumbering &VN;
};

} // namespace llvm::memoir

#endif
