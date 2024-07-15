#include "memoir/utility/InstructionUtils.hpp"

namespace llvm::memoir {

// Base InsertInst implementation.
RESULTANT(InsertInst, ResultCollection)

// SeqInsertInst implementation.
OPERAND(SeqInsertInst, BaseCollection, 0)
OPERAND(SeqInsertInst, InsertionPoint, 1)
TO_STRING(SeqInsertInst)

// SeqInsertValueInst implementation.
OPERAND(SeqInsertValueInst, ValueInserted, 0)
OPERAND(SeqInsertValueInst, BaseCollection, 1)
OPERAND(SeqInsertValueInst, InsertionPoint, 2)
TO_STRING(SeqInsertValueInst)

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
