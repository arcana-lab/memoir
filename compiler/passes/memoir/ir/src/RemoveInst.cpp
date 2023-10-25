#include "memoir/ir/Instructions.hpp"

#include "memoir/support/InstructionUtils.hpp"

namespace llvm::memoir {

// Base RemoveInst implementation.
RESULTANT(RemoveInst, ResultCollection)

// SeqRemoveInst implementation.
OPERAND(SeqRemoveInst, BaseCollection, 0)
OPERAND(SeqRemoveInst, BeginIndex, 1)
OPERAND(SeqRemoveInst, EndIndex, 2)

// AssocRemoveInst implementation.
OPERAND(AssocRemoveInst, BaseCollection, 0)
OPERAND(AssocRemoveInst, Key, 1)

} // namespace llvm::memoir
