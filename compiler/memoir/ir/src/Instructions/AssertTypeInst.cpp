#include "memoir/ir/Instructions.hpp"
#include "memoir/ir/TypeCheck.hpp"
#include "memoir/utility/InstructionUtils.hpp"

namespace llvm::memoir {

// AssertTypeInst implementation
Type &AssertTypeInst::getType() const {
  return MEMOIR_SANITIZE(type_of(this->getTypeOperand()),
                         "Failed to get type used by AssertStructType!");
}

OPERAND(AssertTypeInst, TypeOperand, 0)
OPERAND(AssertTypeInst, Object, 1)
TO_STRING(AssertTypeInst)

} // namespace llvm::memoir
