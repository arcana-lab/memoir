#include "memoir/ir/Instructions.hpp"

#include "memoir/analysis/TypeAnalysis.hpp"

#include "memoir/utility/InstructionUtils.hpp"

namespace llvm::memoir {

// AssertStructTypeInst implementation
Type &AssertStructTypeInst::getType() const {
  return *(TypeAnalysis::get().getType(this->getTypeOperand()));
}

OPERAND(AssertStructTypeInst, TypeOperand, 0)
OPERAND(AssertStructTypeInst, Struct, 1)
TO_STRING(AssertStructTypeInst)

// AssertCollectionTypeInst implementation
Type &AssertCollectionTypeInst::getType() const {
  auto type = TypeAnalysis::analyze(this->getTypeOperand());
  MEMOIR_NULL_CHECK(type, "Could not determine type to assert");
  return *type;
}

OPERAND(AssertCollectionTypeInst, TypeOperand, 0)
OPERAND(AssertCollectionTypeInst, Collection, 1)
TO_STRING(AssertCollectionTypeInst)

} // namespace llvm::memoir
