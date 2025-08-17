#include "memoir/ir/Instructions.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/utility/InstructionUtils.hpp"

#include "memoir/utility/Metadata.hpp"

namespace llvm::memoir {

// UsePHIInst implementation
RESULTANT(UsePHIInst, Result)
OPERAND(UsePHIInst, Used, 0)

TO_STRING(UsePHIInst, "usephi")

// RetPHIInst implementation
RESULTANT(RetPHIInst, Result)
OPERAND(RetPHIInst, Input, 0)

llvm::Function *RetPHIInst::getCalledFunction() const {
  return dyn_cast<llvm::Function>(&this->getCalledOperand());
}

OPERAND(RetPHIInst, CalledOperand, 1)
TO_STRING(RetPHIInst, "retphi")

} // namespace llvm::memoir
