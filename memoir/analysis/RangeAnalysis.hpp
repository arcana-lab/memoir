#ifndef MEMOIR_ANALYSIS_RANGEANALYSIS_H
#define MEMOIR_ANALYSIS_RANGEANALYSIS_H

// LLVM
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"

// NOELLE
#include "noelle/core/Noelle.hpp"

// MemOIR
#include "memoir/passes/Passes.hpp"

#include "memoir/analysis/ValueExpression.hpp"

#include "memoir/support/DataTypes.hpp"

/*
 * This file contains a Symbolic, Intraprocedural Range Analysis.
 *
 * Author(s): Tommy McMichen
 * Created: July 4, 2023
 */

namespace memoir {

struct ValueRange;

struct RangeAnalysisResult {
public:
  /**
   * Queries the value range for the given LLVM Use @use.
   */
  ValueRange &get_value_range(llvm::Use &use);

  /**
   * Prints the results of the Range Analysis.
   */
  void dump();

  ~RangeAnalysisResult();

  friend class RangeAnalysisDriver;

protected:
  // Owned state.
  Set<ValueRange *> ranges;

  // Borrowed state.
  Map<llvm::Use *, ValueRange *> use_to_range;
};

/**
 * Range analysis driver.
 * Will analyze the given function on initilization.
 * Results can be queried with `getValueRange`.
 */
class RangeAnalysisDriver {
public:
  /**
   * Construct a new intraprocedural range analysis.
   * Requires abstractions from NOELLE.
   */
  RangeAnalysisDriver(llvm::Module &M,
                      arcana::noelle::Noelle &noelle,
                      RangeAnalysisResult &result);

  /**
   * Queries the value range for the given LLVM Use @use.
   */
  ValueRange &get_value_range(llvm::Use &use);

protected:
  void propagate_range_to_uses(ValueRange &range, const Set<llvm::Use *> &uses);

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

  // Borrowed state.
  llvm::Module &M;
  arcana::noelle::Noelle &noelle;
  RangeAnalysisResult &result;

  // Analysis driver.
  bool analyze(llvm::Module &M, arcana::noelle::Noelle &noelle);

public:
  ~RangeAnalysisDriver() {}
};

/**
 * Value Range result.
 * Represents the lower and upper bound of a value range as ValueExpressions.
 */
struct ValueRange {
public:
  /**
   * Construct a new value range of [@lower,@upper).
   */
  ValueRange(ValueExpression &lower, ValueExpression &upper)
    : _lower(lower),
      _upper(upper) {}

  /**
   * Get the Value Expression for the lower range.
   */
  ValueExpression &get_lower() const {
    return this->_lower;
  }

  /**
   * Get the Value Expression for the upper range.
   */
  ValueExpression &get_upper() const {
    return this->_upper;
  }

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                       const ValueRange &VR) {
    os << "[" << VR.get_lower() << ":" << VR.get_upper() << "]";
    return os;
  }

protected:
  ValueExpression &_lower;
  ValueExpression &_upper;

  friend class RangeAnalysisDriver;
};

} // namespace memoir

#endif // MEMOIR_ANALYSIS_RANGEANALYSIS_H
