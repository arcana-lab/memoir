#ifndef FOLIO_CONSTRAINTS_H
#define FOLIO_CONSTRAINTS_H

#include <string>

#include "memoir/support/TypeTraits.hpp"

#include "memoir/ir/Instructions.hpp"

namespace folio {

// Constraint hierarchy.
enum ConstraintKind {
#define CONSTRAINT(ENUM, CLASS, IS_SOFT) CONSTRAINT_##ENUM,
#include "folio/analysis/Constraints.def"
#undef CONSTRAINT
};

struct Constraint {
public:
  Constraint(ConstraintKind kind) : kind(kind) {}

  bool operator<(const Constraint &other) const {
    return this->kind < other.kind;
  }

protected:
  ConstraintKind kind;
};

template <typename Inst,
          std::enable_if_t<std::is_base_of_v<llvm::memoir::MemOIRInst, Inst>,
                           bool> = true>
struct OperationConstraint : public Constraint {

  static std::string name();
  static bool is_soft();

  OperationConstraint();
};

template <typename Inst,
          std::enable_if_t<std::is_base_of_v<llvm::memoir::MemOIRInst, Inst>,
                           bool> = true>
struct FastOperationConstraint : public Constraint {

  static std::string name();
  static bool is_soft();

  FastOperationConstraint();
};

struct PointerStableConstraint : public Constraint {

  static std::string name();
  static bool is_soft();

  PointerStableConstraint();
};
} // namespace folio

#endif // FOLIO_CONSTRAINTS_H
