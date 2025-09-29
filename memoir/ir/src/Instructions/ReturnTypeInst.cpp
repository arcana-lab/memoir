#include "memoir/ir/Instructions.hpp"

#include "memoir/ir/TypeCheck.hpp"

#include "memoir/utility/InstructionUtils.hpp"

namespace memoir {

// ReturnTypeInst implementation
Type &ReturnTypeInst::getType() const {
  auto type = type_of(this->getTypeOperand());
  MEMOIR_NULL_CHECK(type, "Could not determine the return type");
  return *type;
}

OPERAND(ReturnTypeInst, TypeOperand, 0)
TO_STRING(ReturnTypeInst, "rettype")

} // namespace memoir
