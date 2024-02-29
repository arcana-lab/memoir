#include "memoir/ir/Instructions.hpp"

#include "memoir/utility/InstructionUtils.hpp"

namespace llvm::memoir {

// Base CopyInst implementation.
RESULTANT(CopyInst, Copy)
OPERAND(CopyInst, CopiedCollection, 0)

// SeqCopyInst implementation.
OPERAND(SeqCopyInst, BeginIndex, 1)
OPERAND(SeqCopyInst, EndIndex, 2)
TO_STRING(SeqCopyInst)

} // namespace llvm::memoir
