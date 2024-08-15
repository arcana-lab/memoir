#ifndef FOLIO_CONSTRAINTINFERENCE_H
#define FOLIO_CONSTRAINTINFERENCE_H

// C++
#include <string>
#include <type_traits>

// LLVM
#include "llvm/Pass.h"

#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Value.h"

// MEMOIR
#include "memoir/ir/Instructions.hpp"

#include "memoir/support/InternalDatatypes.hpp"
#include "memoir/support/TypeTraits.hpp"

// Folio
#include "folio/analysis/Constraints.hpp"

namespace folio {

// Analysis result.
using ConstraintSet = typename llvm::memoir::ordered_set<Constraint>;

struct Constraints {
public:
  using iterator =
      typename llvm::memoir::map<llvm::Value *, ConstraintSet>::iterator;
  using const_iterator =
      typename llvm::memoir::map<llvm::Value *, ConstraintSet>::const_iterator;
  using reference =
      typename llvm::memoir::map<llvm::Value *, ConstraintSet>::mapped_type &;

  // const ConstraintSet &get(llvm::Value &V) const;

  iterator begin() {
    return this->value_to_constraints.begin();
  }

  iterator end() {
    return this->value_to_constraints.end();
  }

  const_iterator cbegin() const {
    return this->value_to_constraints.cbegin();
  }

  const_iterator cend() const {
    return this->value_to_constraints.cend();
  }

  reference operator[](llvm::Value &V) {
    return this->value_to_constraints[&V];
  }

protected:
  llvm::memoir::map<llvm::Value *, ConstraintSet> value_to_constraints;

  void add(llvm::Value &V, Constraint C) {
    this->value_to_constraints[&V].insert(C);
  }

  friend class ConstraintInference;
  friend class ConstraintInferenceDriver;
};

// Driver
class ConstraintInferenceDriver {
public:
  ConstraintInferenceDriver(Constraints &constraints, llvm::Module &M);

protected:
  /**
   * Propagate the constraint to the given value.
   *
   * @param constraints the state
   * @param V the value being propagated to
   * @param C the constraint being propagated
   * @returns true if V's constraints were changed, false otherwise
   */
  bool propagate(llvm::Value &V, ConstraintSet &C);

  void init();

  void infer();

  Constraints &constraints;
  llvm::Module &M;
};

// Analysis.
class ConstraintInference
  : public llvm::AnalysisInfoMixin<ConstraintInference> {

  friend struct llvm::AnalysisInfoMixin<ConstraintInference>;

  static llvm::AnalysisKey Key;

public:
  using Result = Constraints;

  Result run(llvm::Module &M, llvm::ModuleAnalysisManager &MAM);
};

} // namespace folio

#endif // FOLIO_CONSTRAINTINFERENCE_H
