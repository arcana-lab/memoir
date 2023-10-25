#include "memoir/ir/Instructions.hpp"

#include "memoir/ir/InstructionUtils.hpp"

namespace llvm::memoir {

// Base CopyInst implementation.
RESULTANT(CopyInst, Copy)
OPERAND(CopyInst, CopiedCollection, 0)

// SeqCopyInst implementation.
OPERAND(CopyInst, BeginIndex, 1)
OPERAND(CopyInst, EndIndex, 2)
TO_STRING(CopyInst)

} // namespace llvm::memoir
