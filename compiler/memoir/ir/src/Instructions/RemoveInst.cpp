#include "memoir/ir/Instructions.hpp"

#include "memoir/utility/InstructionUtils.hpp"

namespace llvm::memoir {

// Base RemoveInst implementation.
RESULTANT(RemoveInst, ResultCollection)

// SeqRemoveInst implementation.
OPERAND(SeqRemoveInst, BaseCollection, 0)
OPERAND(SeqRemoveInst, BeginIndex, 1)
OPERAND(SeqRemoveInst, EndIndex, 2)
TO_STRING(SeqRemoveInst)

// AssocRemoveInst implementation.
OPERAND(AssocRemoveInst, BaseCollection, 0)
OPERAND(AssocRemoveInst, Key, 1)
TO_STRING(AssocRemoveInst)

} // namespace llvm::memoir
