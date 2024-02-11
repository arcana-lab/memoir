#ifndef MEMOIR_RANGEANALYSIS_H
#define MEMOIR_RANGEANALYSIS_H
#pragma once

// LLVM
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"

// NOELLE
#include "noelle/core/Noelle.hpp"

// MemOIR
#include "memoir/analysis/ValueExpression.hpp"
#include "memoir/support/InternalDatatypes.hpp"

/*
 * This file contains a Symbolic, Intraprocedural Range Analysis.
 *
 * Author(s): Tommy McMichen
 * Created: July 4, 2023
 */

namespace llvm::memoir {

struct ValueRange;

/**
 * Range analysis driver.
 * Will analyze the given function on initilization.
 * Results can be queried with `getValueRange`.
 */
class RangeAnalysis {
public:
  RangeAnalysis(llvm::Function &F, arcana::noelle::Noelle &noelle);
  ~RangeAnalysis();

  ValueRange &get_value_range(llvm::Use &use);

  void dump();

protected:
  void propagate_range_to_uses(ValueRange &range, const set<llvm::Use *> &uses);

  ValueRange &induction_variable_to_range(
      arcana::noelle::LoopGoverningInductionVariable &LGIV);

  ValueRange &induction_variable_to_range(
      arcana::noelle::InductionVariable &IV,
      arcana::noelle::LoopGoverningInductionVariable &LGIV);

  ValueRange &create_value_range(ValueExpression &lower,
                                 ValueExpression &upper);

  ValueExpression &create_min_expr();

  ValueExpression &create_max_expr();

  ValueRange &create_overdefined_range();

  // Owned state.
  set<ValueRange *> ranges;
  map<llvm::Use *, ValueRange *> use_to_range;

  // Borrowed state.
  llvm::Function &F;
  arcana::noelle::Noelle &noelle;

  // Analysis driver.
  bool analyze(llvm::Function &F, arcana::noelle::Noelle &noelle);
};

/**
 * Value Range result.
 * Represents the lower and upper bound of a value range as ValueExpressions.
 */
struct ValueRange {
public:
  ValueExpression &get_lower() const {
    return this->_lower;
  }

  ValueExpression &get_upper() const {
    return this->_upper;
  }

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                       const ValueRange &VR) {
    os << "[" << VR.get_lower() << ":" << VR.get_upper() << "]";
    return os;
  }

protected:
  ValueRange(ValueExpression &lower, ValueExpression &upper)
    : _lower(lower),
      _upper(upper) {}

  ValueExpression &_lower;
  ValueExpression &_upper;

  friend class RangeAnalysis;
};

} // namespace llvm::memoir

#endif
