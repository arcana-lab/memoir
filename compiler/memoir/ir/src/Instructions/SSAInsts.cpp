#include "memoir/ir/Instructions.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/utility/InstructionUtils.hpp"

#include "memoir/utility/Metadata.hpp"

namespace llvm::memoir {

// UsePHIInst implementation
RESULTANT(UsePHIInst, ResultCollection)
OPERAND(UsePHIInst, UsedCollection, 0)

TO_STRING(UsePHIInst)

// DefPHIInst implementation
RESULTANT(DefPHIInst, ResultCollection)
OPERAND(DefPHIInst, DefinedCollection, 0)

TO_STRING(DefPHIInst)

// ArgPHIInst implementation
RESULTANT(ArgPHIInst, ResultCollection)
OPERAND(ArgPHIInst, InputCollection, 0)
// TODO: implement metadata for storing the incoming collections.
TO_STRING(ArgPHIInst)

// RetPHIInst implementation
RESULTANT(RetPHIInst, ResultCollection)
OPERAND(RetPHIInst, InputCollection, 0)

llvm::Function *RetPHIInst::getCalledFunction() const {
  return dyn_cast<llvm::Function>(&this->getCalledOperand());
}

OPERAND(RetPHIInst, CalledOperand, 1)
// TODO: implement metadata for storing the incoming collections.
TO_STRING(RetPHIInst)

// ClearInst implementation
RESULTANT(ClearInst, ResultCollection)
OPERAND(ClearInst, InputCollection, 0)
TO_STRING(ClearInst)

} // namespace llvm::memoir
