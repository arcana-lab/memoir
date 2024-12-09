#include "llvm/IR/Constants.h"

#include "memoir/ir/MutOperations.hpp"

#include "memoir/ir/Types.hpp"

#include "memoir/utility/InstructionUtils.hpp"

namespace llvm::memoir {

// MutWriteInst implementation
OPERAND(MutWriteInst, ValueWritten, 0)
OPERAND(MutWriteInst, ObjectOperand, 1)

unsigned MutWriteInst::getNumberOfSubIndices() const {
  return (this->getCallInst().arg_size() - 3);
}

vector<llvm::Value *> MutWriteInst::getSubIndices() const {
  auto &call = this->getCallInst();

  vector<llvm::Value *> sub_indices(std::next(call.arg_begin(), 3),
                                    call.arg_end());

  return sub_indices;
}

unsigned MutWriteInst::getSubIndex(unsigned sub_idx) const {
  auto &sub_index_as_value = this->getSubIndexOperand(sub_idx);
  auto sub_index_as_constant = dyn_cast<llvm::ConstantInt>(&sub_index_as_value);
  MEMOIR_NULL_CHECK(sub_index_as_constant,
                    "Attempt to access a struct with non-constant field index");

  auto sub_index = sub_index_as_constant->getZExtValue();

  MEMOIR_ASSERT(
      (sub_index < 256),
      "Attempt to access a struct with more than 255 fields"
      "This is unsupported due to the maximum number of arguments allowed in LLVM CallInsts");

  return (unsigned)sub_index;
}

VAR_OPERAND(MutWriteInst, SubIndexOperand, 3)

Type &MutWriteInst::getElementType() const {
  // Determine the element type from the operation.
  switch (this->getKind()) {
    case MemOIR_Func::MUT_STRUCT_WRITE_UINT64:
    case MemOIR_Func::MUT_INDEX_WRITE_UINT64:
    case MemOIR_Func::MUT_ASSOC_WRITE_UINT64:
      return Type::get_u64_type();
    case MemOIR_Func::MUT_STRUCT_WRITE_UINT32:
    case MemOIR_Func::MUT_INDEX_WRITE_UINT32:
    case MemOIR_Func::MUT_ASSOC_WRITE_UINT32:
      return Type::get_u32_type();
    case MemOIR_Func::MUT_STRUCT_WRITE_UINT16:
    case MemOIR_Func::MUT_INDEX_WRITE_UINT16:
    case MemOIR_Func::MUT_ASSOC_WRITE_UINT16:
      return Type::get_u16_type();
    case MemOIR_Func::MUT_STRUCT_WRITE_UINT8:
    case MemOIR_Func::MUT_INDEX_WRITE_UINT8:
    case MemOIR_Func::MUT_ASSOC_WRITE_UINT8:
      return Type::get_u8_type();
    case MemOIR_Func::MUT_STRUCT_WRITE_INT64:
    case MemOIR_Func::MUT_INDEX_WRITE_INT64:
    case MemOIR_Func::MUT_ASSOC_WRITE_INT64:
      return Type::get_u64_type();
    case MemOIR_Func::MUT_STRUCT_WRITE_INT32:
    case MemOIR_Func::MUT_INDEX_WRITE_INT32:
    case MemOIR_Func::MUT_ASSOC_WRITE_INT32:
      return Type::get_u32_type();
    case MemOIR_Func::MUT_STRUCT_WRITE_INT16:
    case MemOIR_Func::MUT_INDEX_WRITE_INT16:
    case MemOIR_Func::MUT_ASSOC_WRITE_INT16:
      return Type::get_u16_type();
    case MemOIR_Func::MUT_STRUCT_WRITE_INT8:
    case MemOIR_Func::MUT_INDEX_WRITE_INT8:
    case MemOIR_Func::MUT_ASSOC_WRITE_INT8:
      return Type::get_u8_type();
    case MemOIR_Func::MUT_STRUCT_WRITE_INT2:
    case MemOIR_Func::MUT_INDEX_WRITE_INT2:
    case MemOIR_Func::MUT_ASSOC_WRITE_INT2:
      return Type::get_u2_type();
    case MemOIR_Func::MUT_STRUCT_WRITE_BOOL:
    case MemOIR_Func::MUT_INDEX_WRITE_BOOL:
    case MemOIR_Func::MUT_ASSOC_WRITE_BOOL:
      return Type::get_bool_type();
    case MemOIR_Func::MUT_STRUCT_WRITE_DOUBLE:
    case MemOIR_Func::MUT_INDEX_WRITE_DOUBLE:
    case MemOIR_Func::MUT_ASSOC_WRITE_DOUBLE:
      return Type::get_f64_type();
    case MemOIR_Func::MUT_STRUCT_WRITE_FLOAT:
    case MemOIR_Func::MUT_INDEX_WRITE_FLOAT:
    case MemOIR_Func::MUT_ASSOC_WRITE_FLOAT:
      return Type::get_f32_type();
    case MemOIR_Func::MUT_STRUCT_WRITE_PTR:
    case MemOIR_Func::MUT_INDEX_WRITE_PTR:
    case MemOIR_Func::MUT_ASSOC_WRITE_PTR:
      return Type::get_ptr_type();
    default: // Otherwise, analyze the function to determine the type.
      return cast<CollectionType>(type_of(this->getObjectOperand()))
          ->getElementType();
  }
}

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
OPERAND(MutIndexWriteInst, Index, 2)
TO_STRING(MutIndexWriteInst)

// MutAssocWriteInst implementation
OPERAND(MutAssocWriteInst, KeyOperand, 2)
TO_STRING(MutAssocWriteInst)

// MutSeqInsertInst implementation
OPERAND(MutSeqInsertInst, Collection, 0)
OPERAND(MutSeqInsertInst, InsertionPoint, 1)
TO_STRING(MutSeqInsertInst)

// MutSeqInsertValueInst implementation
OPERAND(MutSeqInsertValueInst, ValueInserted, 0)
OPERAND(MutSeqInsertValueInst, Collection, 1)
OPERAND(MutSeqInsertValueInst, InsertionPoint, 2)
TO_STRING(MutSeqInsertValueInst)

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

// MutClearInst implementation
OPERAND(MutClearInst, Collection, 0)
TO_STRING(MutClearInst)

} // namespace llvm::memoir
