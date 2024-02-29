#include "memoir/ir/Instructions.hpp"

#include "memoir/analysis/TypeAnalysis.hpp"

#include "memoir/utility/InstructionUtils.hpp"

namespace llvm::memoir {

// ReturnTypeInst implementation
Type &ReturnTypeInst::getType() const {
  auto type = TypeAnalysis::analyze(this->getTypeOperand());
  MEMOIR_NULL_CHECK(type, "Could not determine the return type");
  return *type;
}

OPERAND(ReturnTypeInst, TypeOperand, 0)
TO_STRING(ReturnTypeInst)

} // namespace llvm::memoir
