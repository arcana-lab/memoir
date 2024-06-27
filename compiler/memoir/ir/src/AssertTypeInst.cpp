#include "memoir/ir/Instructions.hpp"

#include "memoir/ir/TypeCheck.hpp"

#include "memoir/utility/InstructionUtils.hpp"

namespace llvm::memoir {

// AssertStructTypeInst implementation
Type &AssertStructTypeInst::getType() const {
  return MEMOIR_SANITIZE(type_of(this->getTypeOperand()),
                         "Failed to get type used by AssertStructType!");
}

OPERAND(AssertStructTypeInst, TypeOperand, 0)
OPERAND(AssertStructTypeInst, Object, 1)
OPERAND(AssertStructTypeInst, Struct, 1)
TO_STRING(AssertStructTypeInst)

// AssertCollectionTypeInst implementation
Type &AssertCollectionTypeInst::getType() const {
  return MEMOIR_SANITIZE(type_of(this->getTypeOperand()),
                         "Failed to get type used by AssertCollectionType");
}

OPERAND(AssertCollectionTypeInst, TypeOperand, 0)
OPERAND(AssertCollectionTypeInst, Object, 1)
OPERAND(AssertCollectionTypeInst, Collection, 1)
TO_STRING(AssertCollectionTypeInst)

} // namespace llvm::memoir
