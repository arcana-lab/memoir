#include "memoir/ir/Instructions.hpp"

#include "memoir/ir/TypeCheck.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/utility/InstructionUtils.hpp"

namespace llvm::memoir {

// Base AllocInst implementation
RESULTANT(AllocInst, Allocation)

// StructAllocInst implementation
StructType &StructAllocInst::getStructType() const {
  // TODO: convert this to a dyn_cast later.
  return static_cast<StructType &>(this->getType());
}

Type &StructAllocInst::getType() const {
  auto type = type_of(this->getTypeOperand());
  MEMOIR_NULL_CHECK(
      type,
      "TypeAnalysis could not determine type for struct allocation");
  return *type;
}

OPERAND(StructAllocInst, TypeOperand, 0)

TO_STRING(StructAllocInst)

// CollectionAllocInst implementation
Type &CollectionAllocInst::getType() const {
  return this->getCollectionType();
}

// TensorAllocInst implementation
RESULTANT(TensorAllocInst, Collection)

CollectionType &TensorAllocInst::getCollectionType() const {
  return Type::get_tensor_type(this->getElementType(),
                               this->getNumberOfDimensions());
}

Type &TensorAllocInst::getElementType() const {
  auto type = type_of(this->getElementOperand());
  MEMOIR_NULL_CHECK(type, "Could not determine the element type");
  return *type;
}

OPERAND(TensorAllocInst, ElementOperand, 0)

unsigned TensorAllocInst::getNumberOfDimensions() const {
  auto &num_dims_as_value = this->getNumberOfDimensionsOperand();
  auto num_dims_as_constant = dyn_cast<llvm::ConstantInt>(&num_dims_as_value);
  MEMOIR_NULL_CHECK(
      num_dims_as_constant,
      "Attempt to allocate a tensor with dynamic number of dimensions");

  auto num_dims = num_dims_as_constant->getZExtValue();

  MEMOIR_ASSERT(
      (num_dims < 256),
      "Attempt to allocate a tensor with more than 255 dimensions"
      "This is unsupported due to the maximum number of arguments allowed in LLVM CallInsts");

  return (unsigned)num_dims;
}

OPERAND(TensorAllocInst, NumberOfDimensionsOperand, 1)
VAR_OPERAND(TensorAllocInst, LengthOfDimensionOperand, 2)
TO_STRING(TensorAllocInst)

// AssocArrayAllocInst implementation
RESULTANT(AssocArrayAllocInst, Collection)

CollectionType &AssocArrayAllocInst::getCollectionType() const {
  return Type::get_assoc_array_type(this->getKeyType(), this->getValueType());
}

Type &AssocArrayAllocInst::getKeyType() const {
  auto type = type_of(this->getKeyOperand());
  MEMOIR_NULL_CHECK(type, "Could not determine the Key type");
  return *type;
}

OPERAND(AssocArrayAllocInst, KeyOperand, 0)

Type &AssocArrayAllocInst::getValueType() const {
  auto type = type_of(this->getValueOperand());
  MEMOIR_NULL_CHECK(type, "Could not determine the Value type");
  return *type;
}

OPERAND(AssocArrayAllocInst, ValueOperand, 1)

TO_STRING(AssocArrayAllocInst)

// SequenceAllocInst implementation
RESULTANT(SequenceAllocInst, Collection)

CollectionType &SequenceAllocInst::getCollectionType() const {
  return Type::get_sequence_type(this->getElementType());
}

Type &SequenceAllocInst::getElementType() const {
  auto type = type_of(this->getElementOperand());
  MEMOIR_NULL_CHECK(type, "Could not determine the element type");
  return *type;
}

OPERAND(SequenceAllocInst, ElementOperand, 0)

OPERAND(SequenceAllocInst, SizeOperand, 1)

TO_STRING(SequenceAllocInst)

} // namespace llvm::memoir
