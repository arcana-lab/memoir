#include "folio/analysis/ConstraintInference.hpp"

using namespace llvm::memoir;

namespace folio {

// =========================================
// Basic Constraint implementations.
#define CONSTRAINT(NAME, CLASS, IS_SOFT)                                       \
  std::string CLASS::name() {                                                  \
    return #NAME;                                                              \
  }                                                                            \
  bool CLASS::is_soft() {                                                      \
    return IS_SOFT;                                                            \
  }                                                                            \
  CLASS::CLASS() : Constraint(ConstraintKind::CONSTRAINT_##NAME) {}

// =========================================

// =========================================
// OperationConstraint implementation.
#define OP_CONSTRAINT(NAME, CLASS, INST, IS_SOFT)                              \
  template <>                                                                  \
  std::string CLASS<INST>::name() {                                            \
    return #NAME;                                                              \
  }                                                                            \
  template <>                                                                  \
  bool CLASS<INST>::is_soft() {                                                \
    return IS_SOFT;                                                            \
  }                                                                            \
  template <>                                                                  \
  CLASS<INST>::CLASS() : Constraint(ConstraintKind::CONSTRAINT_##NAME) {}

// =========================================

// =========================================
// FastOperationConstraint implementation.
#define FASTOP_CONSTRAINT(NAME, CLASS, INST, IS_SOFT)                          \
  template <>                                                                  \
  std::string CLASS<INST>::name() {                                            \
    return #NAME;                                                              \
  }                                                                            \
  template <>                                                                  \
  bool CLASS<INST>::is_soft() {                                                \
    return IS_SOFT;                                                            \
  }                                                                            \
  template <>                                                                  \
  CLASS<INST>::CLASS() : Constraint(ConstraintKind::CONSTRAINT_##NAME) {}

// =========================================

#include "folio/analysis/Constraints.def"
#undef CONSTRAINT
#undef OP_CONSTRAINT
#undef FASTOP_CONSTRAINT

} // namespace folio
