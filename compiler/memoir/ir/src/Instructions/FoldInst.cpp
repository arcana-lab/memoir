#include "memoir/ir/Instructions.hpp"

#include "memoir/utility/InstructionUtils.hpp"

namespace llvm::memoir {

// FoldInst implementation.
RESULTANT(FoldInst, Result)

OPERAND(FoldInst, Collection, 0)

OPERAND(FoldInst, Initial, 1)

llvm::Function &FoldInst::getFunction() const {
  auto &F =
      MEMOIR_SANITIZE(dyn_cast<llvm::Function>(&this->getFunctionOperand()),
                      "FoldInst passed an indirect function to call!");

  MEMOIR_ASSERT(not F.empty(), "FoldInst passed an empty function to call!");

  return F;
}

OPERAND(FoldInst, FunctionOperand, 2)

unsigned FoldInst::getNumberOfClosed() const {
  return (this->getCallInst().arg_size() - 3);
}

VAR_OPERAND(FoldInst, Closed, 3)

TO_STRING(FoldInst)

} // namespace llvm::memoir
