#include "memoir/utility/InstructionUtils.hpp"

namespace llvm::memoir {

// Base InsertInst implementation.
RESULTANT(InsertInst, ResultCollection)

// SeqInsertInst implementation.
OPERAND(SeqInsertInst, ValueInserted, 0)
OPERAND(SeqInsertInst, BaseCollection, 1)
OPERAND(SeqInsertInst, InsertionPoint, 2)
TO_STRING(SeqInsertInst)

// SeqInsertSeqInst implementation.
OPERAND(SeqInsertSeqInst, InsertedCollection, 0)
OPERAND(SeqInsertSeqInst, BaseCollection, 1)
OPERAND(SeqInsertSeqInst, InsertionPoint, 2)
TO_STRING(SeqInsertSeqInst)

// AssocInsertInst implementation.
OPERAND(AssocInsertInst, BaseCollection, 0)
OPERAND(AssocInsertInst, InsertionPoint, 1)
TO_STRING(AssocInsertInst)

} // namespace llvm::memoir
