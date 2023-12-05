#include "memoir/ir/Instructions.hpp"

#include "memoir/support/Assert.hpp"

#include "memoir/support/InstructionUtils.hpp"

namespace llvm::memoir {

// ViewInst implemenatation
RESULTANT(ViewInst, View)
OPERAND(ViewInst, Collection, 0)
OPERAND(ViewInst, BeginIndex, 1)
OPERAND(ViewInst, EndIndex, 2)
TO_STRING(ViewInst)
} // namespace llvm::memoir
