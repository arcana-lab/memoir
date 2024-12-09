#include "memoir/ir/Instructions.hpp"

#include "memoir/ir/TypeCheck.hpp"

#include "memoir/utility/InstructionUtils.hpp"

namespace llvm::memoir {

// AccessInst implementation
CollectionType &AccessInst::getCollectionType() const {
  auto type = type_of(this->getObjectOperand());
  MEMOIR_NULL_CHECK(type, "Could not determine type of collection being read");

  auto collection_type = dyn_cast<CollectionType>(type);
  MEMOIR_NULL_CHECK(
      collection_type,
      "Type being accessed by read inst is not a collection type!");

  return *collection_type;
}

Type &AccessInst::getElementType() const {
  // Determine the element type from the operation.
  switch (this->getKind()) {
    case MemOIR_Func::STRUCT_READ_UINT64:
    case MemOIR_Func::STRUCT_WRITE_UINT64:
    case MemOIR_Func::INDEX_READ_UINT64:
    case MemOIR_Func::INDEX_WRITE_UINT64:
    case MemOIR_Func::ASSOC_READ_UINT64:
    case MemOIR_Func::ASSOC_WRITE_UINT64:
      return Type::get_u64_type();
    case MemOIR_Func::STRUCT_READ_UINT32:
    case MemOIR_Func::STRUCT_WRITE_UINT32:
    case MemOIR_Func::INDEX_READ_UINT32:
    case MemOIR_Func::INDEX_WRITE_UINT32:
    case MemOIR_Func::ASSOC_READ_UINT32:
    case MemOIR_Func::ASSOC_WRITE_UINT32:
      return Type::get_u32_type();
    case MemOIR_Func::STRUCT_READ_UINT16:
    case MemOIR_Func::STRUCT_WRITE_UINT16:
    case MemOIR_Func::INDEX_READ_UINT16:
    case MemOIR_Func::INDEX_WRITE_UINT16:
    case MemOIR_Func::ASSOC_READ_UINT16:
    case MemOIR_Func::ASSOC_WRITE_UINT16:
      return Type::get_u16_type();
    case MemOIR_Func::STRUCT_READ_UINT8:
    case MemOIR_Func::STRUCT_WRITE_UINT8:
    case MemOIR_Func::INDEX_READ_UINT8:
    case MemOIR_Func::INDEX_WRITE_UINT8:
    case MemOIR_Func::ASSOC_READ_UINT8:
    case MemOIR_Func::ASSOC_WRITE_UINT8:
      return Type::get_u8_type();
    case MemOIR_Func::STRUCT_READ_INT64:
    case MemOIR_Func::STRUCT_WRITE_INT64:
    case MemOIR_Func::INDEX_READ_INT64:
    case MemOIR_Func::INDEX_WRITE_INT64:
    case MemOIR_Func::ASSOC_READ_INT64:
    case MemOIR_Func::ASSOC_WRITE_INT64:
      return Type::get_u64_type();
    case MemOIR_Func::STRUCT_READ_INT32:
    case MemOIR_Func::STRUCT_WRITE_INT32:
    case MemOIR_Func::INDEX_READ_INT32:
    case MemOIR_Func::INDEX_WRITE_INT32:
    case MemOIR_Func::ASSOC_READ_INT32:
    case MemOIR_Func::ASSOC_WRITE_INT32:
      return Type::get_u32_type();
    case MemOIR_Func::STRUCT_READ_INT16:
    case MemOIR_Func::STRUCT_WRITE_INT16:
    case MemOIR_Func::INDEX_READ_INT16:
    case MemOIR_Func::INDEX_WRITE_INT16:
    case MemOIR_Func::ASSOC_READ_INT16:
    case MemOIR_Func::ASSOC_WRITE_INT16:
      return Type::get_u16_type();
    case MemOIR_Func::STRUCT_READ_INT8:
    case MemOIR_Func::STRUCT_WRITE_INT8:
    case MemOIR_Func::INDEX_READ_INT8:
    case MemOIR_Func::INDEX_WRITE_INT8:
    case MemOIR_Func::ASSOC_READ_INT8:
    case MemOIR_Func::ASSOC_WRITE_INT8:
      return Type::get_u8_type();
    case MemOIR_Func::STRUCT_READ_INT2:
    case MemOIR_Func::STRUCT_WRITE_INT2:
    case MemOIR_Func::INDEX_READ_INT2:
    case MemOIR_Func::INDEX_WRITE_INT2:
    case MemOIR_Func::ASSOC_READ_INT2:
    case MemOIR_Func::ASSOC_WRITE_INT2:
      return Type::get_u2_type();
    case MemOIR_Func::STRUCT_READ_BOOL:
    case MemOIR_Func::STRUCT_WRITE_BOOL:
    case MemOIR_Func::INDEX_READ_BOOL:
    case MemOIR_Func::INDEX_WRITE_BOOL:
    case MemOIR_Func::ASSOC_READ_BOOL:
    case MemOIR_Func::ASSOC_WRITE_BOOL:
      return Type::get_bool_type();
    case MemOIR_Func::STRUCT_READ_DOUBLE:
    case MemOIR_Func::STRUCT_WRITE_DOUBLE:
    case MemOIR_Func::INDEX_READ_DOUBLE:
    case MemOIR_Func::INDEX_WRITE_DOUBLE:
    case MemOIR_Func::ASSOC_READ_DOUBLE:
    case MemOIR_Func::ASSOC_WRITE_DOUBLE:
      return Type::get_f64_type();
    case MemOIR_Func::STRUCT_READ_FLOAT:
    case MemOIR_Func::STRUCT_WRITE_FLOAT:
    case MemOIR_Func::INDEX_READ_FLOAT:
    case MemOIR_Func::INDEX_WRITE_FLOAT:
    case MemOIR_Func::ASSOC_READ_FLOAT:
    case MemOIR_Func::ASSOC_WRITE_FLOAT:
      return Type::get_f32_type();
    case MemOIR_Func::STRUCT_READ_PTR:
    case MemOIR_Func::STRUCT_WRITE_PTR:
    case MemOIR_Func::INDEX_READ_PTR:
    case MemOIR_Func::INDEX_WRITE_PTR:
    case MemOIR_Func::ASSOC_READ_PTR:
    case MemOIR_Func::ASSOC_WRITE_PTR:
      return Type::get_ptr_type();
    default: // Otherwise, analyze the function to determine the type.
      return this->getCollectionType().getElementType();
  }
}

// ReadInst implementation
RESULTANT(ReadInst, ValueRead)
OPERAND(ReadInst, ObjectOperand, 0)

unsigned ReadInst::getNumberOfSubIndices() const {
  return (this->getCallInst().arg_size() - 2);
}

vector<llvm::Value *> ReadInst::getSubIndices() const {
  auto &call = this->getCallInst();

  vector<llvm::Value *> sub_indices(std::next(call.arg_begin(), 2),
                                    call.arg_end());

  return sub_indices;
}

unsigned ReadInst::getSubIndex(unsigned sub_dim) const {
  auto &sub_index_as_value = this->getSubIndexOperand(sub_dim);
  auto sub_index_as_constant = dyn_cast<llvm::ConstantInt>(&sub_index_as_value);
  MEMOIR_NULL_CHECK(
      sub_index_as_constant,
      "Attempt to access a struct element with non-constant field index");

  auto sub_index = sub_index_as_constant->getZExtValue();

  MEMOIR_ASSERT(
      (sub_index < 256),
      "Attempt to access a struct element with more than 255 fields"
      "This is unsupported due to the maximum number of arguments allowed in LLVM CallInsts");

  return (unsigned)sub_index;
}

VAR_OPERAND(ReadInst, SubIndexOperand, 2)

// StructReadInst implementation
CollectionType &StructReadInst::getCollectionType() const {
  auto type = type_of(this->getObjectOperand());
  MEMOIR_NULL_CHECK(type, "Could not determine the type being accessed");

  auto struct_type = dyn_cast<StructType>(type);
  MEMOIR_NULL_CHECK(struct_type,
                    "Could not determine the struct type being accessed");

  auto &field_array_type =
      FieldArrayType::get(*struct_type, this->getFieldIndex());

  return field_array_type;
}

unsigned StructReadInst::getFieldIndex() const {
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

OPERAND(StructReadInst, FieldIndexOperand, 1)
TO_STRING(StructReadInst)

// IndexReadInst implementation
OPERAND(IndexReadInst, Index, 1)

TO_STRING(IndexReadInst)

// AssocReadInst implementation
OPERAND(AssocReadInst, KeyOperand, 1)

TO_STRING(AssocReadInst)

// WriteInst implementation
OPERAND(WriteInst, ValueWritten, 0)

OPERAND(WriteInst, ObjectOperand, 1)

unsigned WriteInst::getNumberOfSubIndices() const {
  return (this->getCallInst().arg_size() - 3);
}

vector<llvm::Value *> WriteInst::getSubIndices() const {
  auto &call = this->getCallInst();

  vector<llvm::Value *> sub_indices(std::next(call.arg_begin(), 3),
                                    call.arg_end());

  return sub_indices;
}

unsigned WriteInst::getSubIndex(unsigned sub_dim) const {
  auto &sub_index_as_value = this->getSubIndexOperand(sub_dim);
  auto sub_index_as_constant = dyn_cast<llvm::ConstantInt>(&sub_index_as_value);
  MEMOIR_NULL_CHECK(
      sub_index_as_constant,
      "Attempt to access a struct element with non-constant field index");

  auto sub_index = sub_index_as_constant->getZExtValue();

  MEMOIR_ASSERT(
      (sub_index < 256),
      "Attempt to access a struct element with more than 255 fields"
      "This is unsupported due to the maximum number of arguments allowed in LLVM CallInsts");

  return (unsigned)sub_index;
}

VAR_OPERAND(WriteInst, SubIndexOperand, 3)

// StructWriteInst implementation
CollectionType &StructWriteInst::getCollectionType() const {
  auto type = type_of(this->getObjectOperand());
  MEMOIR_NULL_CHECK(type, "Could not determine the type being accessed");

  auto struct_type = dyn_cast<StructType>(type);
  MEMOIR_NULL_CHECK(struct_type,
                    "Could not determine the struct type being accessed");

  auto &field_array_type =
      FieldArrayType::get(*struct_type, this->getFieldIndex());

  return field_array_type;
}

unsigned StructWriteInst::getFieldIndex() const {
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

OPERAND(StructWriteInst, FieldIndexOperand, 2)

TO_STRING(StructWriteInst)

// IndexWriteInst implementation
RESULTANT(IndexWriteInst, Collection)

OPERAND(IndexWriteInst, Index, 2)

TO_STRING(IndexWriteInst)

// AssocWriteInst implementation
RESULTANT(AssocWriteInst, Collection)

OPERAND(AssocWriteInst, KeyOperand, 2)

TO_STRING(AssocWriteInst)

// GetInst implementation
RESULTANT(GetInst, NestedObject)

OPERAND(GetInst, ObjectOperand, 0)

// StructGetInst implementation
CollectionType &StructGetInst::getCollectionType() const {
  auto type = type_of(this->getObjectOperand());
  MEMOIR_NULL_CHECK(type, "Could not determine the type being accessed");

  auto struct_type = dyn_cast<StructType>(type);
  MEMOIR_NULL_CHECK(struct_type,
                    "Could not determine the struct type being accessed");

  auto &field_array_type =
      FieldArrayType::get(*struct_type, this->getFieldIndex());

  return field_array_type;
}

unsigned StructGetInst::getFieldIndex() const {
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

OPERAND(StructGetInst, FieldIndexOperand, 1)

TO_STRING(StructGetInst)

// IndexGetInst implementation
OPERAND(IndexGetInst, Index, 1)
TO_STRING(IndexGetInst)

// AssocGetInst implementation
OPERAND(AssocGetInst, KeyOperand, 1)
TO_STRING(AssocGetInst)

// AssocHasInst implementation
OPERAND(AssocHasInst, ObjectOperand, 0)
OPERAND(AssocHasInst, KeyOperand, 1)
TO_STRING(AssocHasInst)

} // namespace llvm::memoir
