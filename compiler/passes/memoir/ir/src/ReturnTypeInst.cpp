#include "memoir/ir/Instructions.hpp"

#include "memoir/analysis/TypeAnalysis.hpp"

namespace llvm::memoir {

// ReturnTypeInst implementation
Type &ReturnTypeInst::getType() const {
  auto type = TypeAnalysis::analyze(this->getTypeOperand());
  MEMOIR_NULL_CHECK(type, "Could not determine the return type");
  return *type;
}

OPERAND(ReturnTypeInst, TypeOperand)
TO_STRING(ReturnTypeInst)

} // namespace llvm::memoir
