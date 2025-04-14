#include "memoir/analysis/ValueExpression.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/support/DataTypes.hpp"
#include "memoir/support/Print.hpp"

#include <iostream>
#include <string>

namespace llvm::memoir {

// Helper macro
#define CHECK_OTHER(OTHER, CLASS)                                              \
  if (!isa<CLASS>(OTHER)) {                                                    \
    return false;                                                              \
  }                                                                            \
  const auto &OE = cast<CLASS>(OTHER);

// ConstantExpression
bool ConstantExpression::equals(const ValueExpression &E) const {
  CHECK_OTHER(E, ConstantExpression);
  return (&(this->C) == &(OE.C));
}

// VariableExpression
bool VariableExpression::equals(const ValueExpression &E) const {
  CHECK_OTHER(E, VariableExpression)
  return (&(this->V) == &(OE.V));
}

// ArgumentExpression
bool ArgumentExpression::equals(const ValueExpression &E) const {
  CHECK_OTHER(E, ArgumentExpression);
  return (&(OE.A) == &(this->A));
}

// UnknownExpression
bool UnknownExpression::equals(const ValueExpression &E) const {
  return false;
}

// BasicExpressionpression
bool BasicExpression::equals(const ValueExpression &E) const {
  CHECK_OTHER(E, BasicExpression);
  if ((this->opcode != E.opcode) || (this->getLLVMType() != OE.getLLVMType())
      || (this->getMemOIRType() != OE.getMemOIRType())
      || (this->I->getNumOperands() != OE.I->getNumOperands())) {
    return false;
  }
  return false;
};

// PHIExpression
bool PHIExpression::equals(const ValueExpression &E) const {
  CHECK_OTHER(E, PHIExpression);
  // TODO
  return false;
}

// SizeExpression
bool SizeExpression::equals(const ValueExpression &E) const {
  CHECK_OTHER(E, SizeExpression);
  // TODO
  return false;
}

// EndExpression
bool EndExpression::equals(const ValueExpression &E) const {
  CHECK_OTHER(E, EndExpression);
  // TODO
  return false;
}

} // namespace llvm::memoir
