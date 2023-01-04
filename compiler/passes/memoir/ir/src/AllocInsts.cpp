#include "memoir/ir/Instructions.hpp"

#include "memoir/analysis/CollectionAnalysis.hpp"
#include "memoir/analysis/StructAnalysis.hpp"
#include "memoir/analysis/TypeAnalysis.hpp"

#include "memoir/support/Assert.hpp"

namespace llvm::memoir {

llvm::Value &AllocInst::getAllocation() const {
  return this->getCallInst();
}

/*
 * StructAllocInst implementation
 */
Struct &StructAllocInst::getStruct() const {
  auto strct = StructAnalysis::analyze(this->getTypeOperand());
  MEMOIR_NULL_CHECK(strct, "StructAnalysis returned NULL for StructAllocInst");

  return *strct;
}

StructType &StructAllocInst::getStructType() const {
  // TODO: convert this to a dyn_cast later.
  return static_cast<StructType &>(this->getType());
}

Type &StructAllocInst::getType() const {
  auto type = TypeAnalysis::analyze(this->getTypeOperand());
  MEMOIR_NULL_CHECK(
      type,
      "TypeAnalysis could not determine type for struct allocation");
  return *type;
}

llvm::Value &StructAllocInst::getTypeOperand() const {
  return *(this->getTypeOperandAsUse().get());
}

llvm::Use &StructAllocInst::getTypeOperandAsUse() const {
  return this->getCallInst().getOperandUse(0);
}

std::string StructAllocInst::toString(std::string indent) const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "StructAllocInst: " + llvm_str;

  return str;
}

/*
 * CollectionAllocInst implementation
 */
Type &CollectionAllocInst::getType() const {
  return this->getCollectionType();
}

/*
 * TensorAllocInst implementation
 */
Collection &TensorAllocInst::getCollection() const {
  auto collection = CollectionAnalysis::analyze(this->getCallInst());
  MEMOIR_NULL_CHECK(collection,
                    "Could not determine the Collection being allocated");
  return *collection;
}

CollectionType &TensorAllocInst::getCollectionType() const {
  return Type::get_tensor_type(this->getElementType(),
                               this->getNumberOfDimensions());
}

Type &TensorAllocInst::getElementType() const {
  auto type = TypeAnalysis::get().getType(this->getElementOperand());
  MEMOIR_NULL_CHECK(type, "Could not determine the element type");
  return *type;
}

llvm::Value &TensorAllocInst::getElementOperand() const {
  return *(this->getElementOperandAsUse().get());
}

llvm::Use &TensorAllocInst::getElementOperandAsUse() const {
  return this->getCallInst().getOperandUse(0);
}

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

llvm::Value &TensorAllocInst::getNumberOfDimensionsOperand() const {
  return *(this->getNumberOfDimensionsOperandAsUse().get());
}

llvm::Use &TensorAllocInst::getNumberOfDimensionsOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(1);
}

llvm::Value &TensorAllocInst::getLengthOfDimensionOperand(
    unsigned dimension_index) const {
  return *(this->getLengthOfDimensionOperandAsUse(dimension_index).get());
}

llvm::Use &TensorAllocInst::getLengthOfDimensionOperandAsUse(
    unsigned dimension_index) const {
  MEMOIR_ASSERT((dimension_index < this->getNumberOfDimensions()),
                "Attempt to get length of out-of-bounds dimensions");
  return this->getCallInst().getArgOperandUse(2 + dimension_index);
}

std::string TensorAllocInst::toString(std::string indent) const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "TensorAllocInst: " + llvm_str;

  return str;
}

/*
 * AssocArrayAllocInst implementation
 */
Collection &AssocArrayAllocInst::getCollection() const {
  auto collection = CollectionAnalysis::analyze(this->getCallInst());
  MEMOIR_NULL_CHECK(collection,
                    "Could not determine the Collection being allocated");
  return *collection;
}

CollectionType &AssocArrayAllocInst::getCollectionType() const {
  return Type::get_assoc_array_type(this->getKeyType(), this->getValueType());
}

Type &AssocArrayAllocInst::getKeyType() const {
  auto type = TypeAnalysis::get().getType(this->getKeyOperand());
  MEMOIR_NULL_CHECK(type, "Could not determine the Key type");
  return *type;
}

llvm::Value &AssocArrayAllocInst::getKeyOperand() const {
  return *(this->getKeyOperandAsUse().get());
}

llvm::Use &AssocArrayAllocInst::getKeyOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

Type &AssocArrayAllocInst::getValueType() const {
  auto type = TypeAnalysis::get().getType(this->getValueOperand());
  MEMOIR_NULL_CHECK(type, "Could not determine the Value type");
  return *type;
}

llvm::Value &AssocArrayAllocInst::getValueOperand() const {
  return *(this->getValueOperandAsUse().get());
}

llvm::Use &AssocArrayAllocInst::getValueOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(1);
}

std::string AssocArrayAllocInst::toString(std::string indent) const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "AssocArrayAllocInst: " + llvm_str;

  return str;
}

/*
 * SequenceAllocInst implementation
 */
Collection &SequenceAllocInst::getCollection() const {
  auto collection = CollectionAnalysis::analyze(this->getCallInst());
  MEMOIR_NULL_CHECK(collection,
                    "Could not determine the Collection being allocated");
  return *collection;
}

CollectionType &SequenceAllocInst::getCollectionType() const {
  return Type::get_sequence_type(this->getElementType());
}

Type &SequenceAllocInst::getElementType() const {
  auto type = TypeAnalysis::get().getType(this->getElementOperand());
  MEMOIR_NULL_CHECK(type, "Could not determine the element type");
  return *type;
}

llvm::Value &SequenceAllocInst::getElementOperand() const {
  return *(this->getElementOperandAsUse().get());
}

llvm::Use &SequenceAllocInst::getElementOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

llvm::Value &SequenceAllocInst::getSizeOperand() const {
  return *(this->getSizeOperandAsUse().get());
}

llvm::Use &SequenceAllocInst::getSizeOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(1);
}

std::string SequenceAllocInst::toString(std::string indent) const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "SequenceAllocInst: " + llvm_str;

  return str;
}

} // namespace llvm::memoir
