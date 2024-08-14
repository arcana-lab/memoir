#include "folio/analysis/ConstraintInference.hpp"

using namespace llvm::memoir;

namespace folio {

// =========================================
// Constraint implementation.
std::string Constraint::name() const {
  switch (this->kind) {
#define CONSTRAINT(NAME, CLASS, IS_SOFT)                                       \
  case ConstraintKind::CONSTRAINT_##NAME:                                      \
    return #NAME;
#include "folio/analysis/Constraints.def"
    default:
      MEMOIR_UNREACHABLE("Unknown constraint!");
  }
}

bool Constraint::is_soft() const {
  switch (this->kind) {
#define CONSTRAINT(NAME, CLASS, IS_SOFT)                                       \
  case ConstraintKind::CONSTRAINT_##NAME:                                      \
    return IS_SOFT;
#include "folio/analysis/Constraints.def"
    default:
      MEMOIR_UNREACHABLE("Unknown constraint!");
  }
}
// =========================================

// =========================================
// Basic Constraint implementations.
#define CONSTRAINT(NAME, CLASS, IS_SOFT)                                       \
  CLASS::CLASS() : Constraint(ConstraintKind::CONSTRAINT_##NAME) {}
// =========================================

// =========================================
// OperationConstraint implementation.
#define OP_CONSTRAINT(NAME, CLASS, INST, IS_SOFT)                              \
  template <>                                                                  \
  CLASS<INST>::CLASS() : Constraint(ConstraintKind::CONSTRAINT_##NAME) {}
// =========================================

// =========================================
// FastOperationConstraint implementation..
#define FASTOP_CONSTRAINT(NAME, CLASS, INST, IS_SOFT)                          \
  template <>                                                                  \
  CLASS<INST>::CLASS() : Constraint(ConstraintKind::CONSTRAINT_##NAME) {}
// =========================================
#include "folio/analysis/Constraints.def"

} // namespace folio
