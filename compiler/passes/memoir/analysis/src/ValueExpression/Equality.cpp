#include "memoir/analysis/ValueExpression.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/support/InternalDatatypes.hpp"
#include "memoir/support/Print.hpp"

namespace llvm::memoir {

// Equality
bool ValueExpression::operator==(const ValueExpression &E) const {
  return this->equals(E);
};

bool ValueExpression::operator!=(const ValueExpression &E) const {
  return !(*this == E);
};

bool ValueExpression::equals(const llvm::Value &V) const {
  return (this->value == &V);
};

bool ValueExpression::operator==(const llvm::Value &Other) const {
  return this->equals(Other);
};

bool ValueExpression::operator!=(const llvm::Value &Other) const {
  return !(*this == Other);
};

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
  CHECK_OTHER(E, UnknownExpression);
  // TODO
  return false;
}

// BasicExpression
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

// CollectionExpression
bool CollectionExpression::equals(const ValueExpression &E) const {
  CHECK_OTHER(E, CollectionExpression);
  // TODO
  return false;
}

// StructExpression
bool StructExpression::equals(const ValueExpression &E) const {
  CHECK_OTHER(E, StructExpression);
  // TODO
  return false;
}

// SizeExpression
bool SizeExpression::equals(const ValueExpression &E) const {
  CHECK_OTHER(E, SizeExpression);
  // TODO
  return false;
}

} // namespace llvm::memoir
