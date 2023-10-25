#include "memoir/ir/Instructions.hpp"

#include "memoir/analysis/CollectionAnalysis.hpp"
#include "memoir/analysis/StructAnalysis.hpp"
#include "memoir/analysis/TypeAnalysis.hpp"

namespace llvm::memoir {

// AccessInst implementation
CollectionType &AccessInst::getCollectionType() const {
  auto type = TypeAnalysis::analyze(this->getObjectOperand());
  MEMOIR_NULL_CHECK(type, "Could not determine type of collection being read");

  auto collection_type = dyn_cast<CollectionType>(type);
  MEMOIR_NULL_CHECK(
      collection_type,
      "Type being accessed by read inst is not a collection type!");

  return *collection_type;
}

// ReadInst implementation
RESULT(ReadInst, ValueRead)
OPERAND(ReadInst, ObjectOperand, 0)

// StructReadInst implementation
CollectionType &StructReadInst::getCollectionType() const {
  auto type = TypeAnalysis::analyze(this->getObjectOperand());
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
unsigned IndexReadInst::getNumberOfDimensions() const {
  return (this->getCallInst().arg_size() - 1);
}
VAR_OPERAND(IndexReadInst, IndexOfDimensions, 1)

TO_STRING(IndexReadInst)

// AssocReadInst implementation
OPERAND(AssocReadInst, KeyOperand, 1)

TO_STRING(AssocReadInst)

// WriteInst implementation
OPERAND(WriteInst, ValueWritten, 0)

OPERAND(WriteInst, ObjectOperand, 1)

// StructWriteInst implementation
CollectionType &StructWriteInst::getCollectionType() const {
  auto type = TypeAnalysis::analyze(this->getObjectOperand());
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

unsigned IndexWriteInst::getNumberOfDimensions() const {
  return (this->getCallInst().arg_size() - 2);
}

VAR_OPERAND(IndexWriteInst, IndexOfDimension, 2)

TO_STRING(IndexWriteInst)

// AssocWriteInst implementation
OPERAND(AssocWriteInst, Collection)

OPERAND(AssocWriteInst, KeyOperand, 2)

TO_STRING(AssocWriteInst)

// GetInst implementation
RESULTANT(GetInst, NestedObject)

OPERAND(GetInst, ObjectOperand, 0)

// StructGetInst implementation
CollectionType &StructGetInst::getCollectionType() const {
  auto type = TypeAnalysis::analyze(this->getObjectOperand());
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
unsigned IndexGetInst::getNumberOfDimensions() const {
  return (this->getCallInst().arg_size() - 1);
}

VAR_OPERAND(IndexGetInst, IndexOfDimension, 1)

TO_STRING(IndexGetInst)

// AssocGetInst implementation
OPERAND(AssocGetInst, KeyOperand, 1)
TO_STRING(AssocGetInst)

// AssocHasInst implementation
OPERAND(AssocHasInst, ObjectOperand, 0)
OPERAND(AssocHasInst, KeyOperand, 1)
TO_STRING(AssocHasInst)

} // namespace llvm::memoir
