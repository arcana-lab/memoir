#include "llvm/IR/Constants.h"

#include "memoir/ir/MutOperations.hpp"

#include "memoir/ir/Types.hpp"

#include "memoir/utility/InstructionUtils.hpp"

namespace llvm::memoir {

// MutWriteInst implementation
OPERAND(MutWriteInst, ValueWritten, 0)
OPERAND(MutWriteInst, ObjectOperand, 1)

// MutStructWriteInst implementation
unsigned MutStructWriteInst::getFieldIndex() const {
  auto &field_index_as_value = this->getFieldIndexOperand();
  auto field_index_as_constant =
      dyn_cast<llvm::ConstantInt>(&field_index_as_value);
  MEMOIR_NULL_CHECK(field_index_as_constant,
                    "Attempt to access a struct with non-constant field index");

  auto field_index = field_index_as_constant->getZExtValue();

  MEMOIR_ASSERT(
      (field_index < 256),
      "Attempt to access a tensor with more than 255 fields"
      "This is unsupported due to the maximum number of arguments allowed in LLVM CallInsts");

  return (unsigned)field_index;
}
OPERAND(MutStructWriteInst, FieldIndexOperand, 2)
TO_STRING(MutStructWriteInst)

// MutIndexWriteInst implementation
unsigned MutIndexWriteInst::getNumberOfDimensions() const {
  return (this->getCallInst().arg_size() - 2);
}

VAR_OPERAND(MutIndexWriteInst, IndexOfDimension, 2)
TO_STRING(MutIndexWriteInst)

// MutAssocWriteInst implementation
OPERAND(MutAssocWriteInst, KeyOperand, 2)
TO_STRING(MutAssocWriteInst)

// MutSeqInsertInst implementation
OPERAND(MutSeqInsertInst, ValueInserted, 0)
OPERAND(MutSeqInsertInst, Collection, 1)
OPERAND(MutSeqInsertInst, InsertionPoint, 2)
TO_STRING(MutSeqInsertInst)

// MutSeqInsertSeqInst implementation
OPERAND(MutSeqInsertSeqInst, InsertedCollection, 0)
OPERAND(MutSeqInsertSeqInst, Collection, 1)
OPERAND(MutSeqInsertSeqInst, InsertionPoint, 2)
TO_STRING(MutSeqInsertSeqInst)

// MutSeqRemoveInst implementation.
OPERAND(MutSeqRemoveInst, Collection, 0)
OPERAND(MutSeqRemoveInst, BeginIndex, 1)
OPERAND(MutSeqRemoveInst, EndIndex, 2)
TO_STRING(MutSeqRemoveInst)

// MutSeqAppendInst implementation.
OPERAND(MutSeqAppendInst, Collection, 0)
OPERAND(MutSeqAppendInst, AppendedCollection, 1)
TO_STRING(MutSeqAppendInst)

// MutSeqSwapInst implementation.
OPERAND(MutSeqSwapInst, FromCollection, 0)
OPERAND(MutSeqSwapInst, BeginIndex, 1)
OPERAND(MutSeqSwapInst, EndIndex, 2)
OPERAND(MutSeqSwapInst, ToCollection, 3)
OPERAND(MutSeqSwapInst, ToBeginIndex, 4)
TO_STRING(MutSeqSwapInst)

// MutSeqSwapWithinInst implementation.
OPERAND(MutSeqSwapWithinInst, FromCollection, 0)
OPERAND(MutSeqSwapWithinInst, ToCollection, 0)
OPERAND(MutSeqSwapWithinInst, BeginIndex, 1)
OPERAND(MutSeqSwapWithinInst, EndIndex, 2)
OPERAND(MutSeqSwapWithinInst, ToBeginIndex, 3)
TO_STRING(MutSeqSwapWithinInst)

// MutSeqSplitInst implementation.
RESULTANT(MutSeqSplitInst, Split)
OPERAND(MutSeqSplitInst, Collection, 0)
OPERAND(MutSeqSplitInst, BeginIndex, 1)
OPERAND(MutSeqSplitInst, EndIndex, 2)
TO_STRING(MutSeqSplitInst)

// MutAssocRemoveInst implementation.
OPERAND(MutAssocRemoveInst, Collection, 0)
OPERAND(MutAssocRemoveInst, KeyOperand, 1)
TO_STRING(MutAssocRemoveInst)

// MutAssocInsertInst implementation.
OPERAND(MutAssocInsertInst, Collection, 0)
OPERAND(MutAssocInsertInst, KeyOperand, 1)
TO_STRING(MutAssocInsertInst)

} // namespace llvm::memoir
