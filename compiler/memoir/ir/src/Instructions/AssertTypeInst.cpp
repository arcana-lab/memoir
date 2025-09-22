#include "memoir/ir/Instructions.hpp"
#include "memoir/ir/TypeCheck.hpp"
#include "memoir/utility/InstructionUtils.hpp"

namespace memoir {

// AssertTypeInst implementation
Type &AssertTypeInst::getType() const {
  return MEMOIR_SANITIZE(type_of(this->getTypeOperand()),
                         "Failed to get type used by AssertTupleType!");
}

OPERAND(AssertTypeInst, TypeOperand, 0)
OPERAND(AssertTypeInst, Object, 1)

std::string AssertTypeInst::toString() const {
  return "assert " + value_name(this->getObject()) + " isa "
         + this->getType().toString();
}

} // namespace memoir
