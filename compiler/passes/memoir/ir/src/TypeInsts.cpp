#include "Instructions.hpp"

#include <limits>

/*
 * Implementation of the MemOIR Type Instructions.
 *
 * Author(s): Tommy McMichen
 * Created: December 13, 2022
 */

/*
 * IntegerTypeInst implementation
 */
Type &IntegerTypeInst::getType() const {
  return TypeAnalysis::get().getType(this->getCallInst());
}

unsigned IntegerTypeInst::getBitwidth() const {
  auto bitwidth_value = this->getBitwidthOperand();
  auto bitwidth_const = dyn_cast<ConstantInt>(&bitwidth_value);
  MEMOIR_NULL_CHECK(bitwidth_const,
                    "Attempt to create Integer Type of non-static bitwidth");

  return bitwidth_const->getZExtValue();
}

llvm::Value &IntegerTypeInst::getBitwidthOperand() const {
  return this->getBitwidthOperandAsUse().get();
}

llvm::Use &IntegerTypeInst::getBitwidthOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

unsigned IntegerTypeInst::isSigned() const {
  auto is_signed_value = this->getIsSignedOperand();
  auto is_signed_const = dyn_cast<ConstantInt>(&is_signed_value);
  MEMOIR_NULL_CHECK(is_signed_const,
                    "Attempt to create Integer Type of non-static sign");

  return is_signed_const->isOne();
}

llvm::Value &IntegerTypeInst::getIsSignedOperand() const {
  return this->getIsSignedOperandAsUse().get();
}

llvm::Use &IntegerTypeInst::getIsSignedOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(1);
}

std::string IntegerTypeInst::toString(std::string indent = "") const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "IntegerTypeInst: " + llvm_str;

  return str;
}

/*
 * FloatType implementation
 */
Type &FloatTypeInst::getType() const {
  return TypeAnalysis::get().getType(this->getCallInst());
}

std::string FloatTypeInst::toString(std::string indent = "") const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "FloatTypeInst: " + llvm_str;

  return str;
}

/*
 * DoubleType implementation
 */
Type &DoubleTypeInst::getType() const {
  return TypeAnalysis::get().getType(this->getCallInst());
}

std::string DoubleTypeInst::toString(std::string indent = "") const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "DoubleTypeInst: " + llvm_str;

  return str;
}

/*
 * PointerType implementation
 */
Type &PointerTypeInst::getType() const {
  return TypeAnalysis::get().getType(this->getCallInst());
}

std::string PointerTypeInst::toString(std::string indent = "") const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "PointerTypeInst: " + llvm_str;

  return str;
}

/*
 * ReferenceType implementation
 */
Type &ReferenceTypeInst::getType() const {
  return TypeAnalysis::get().getType(this->getCallInst());
}

Type &ReferenceTypeInst::getReferencedType() const {
  return TypeAnalysis::get().getType(this->getReferencedTypeOperand());
}

llvm::Value &ReferenceTypeInst::getReferencedTypeOperand() const {
  return this->getReferencedTypeAsUse().get();
}

llvm::Use &ReferenceTypeInst::getReferencedTypeAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

std::string ReferenceTypeInst::toString(std::string indent = "") const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "ReferenceTypeInst: " + llvm_str;

  return str;
}

/*
 * DefineStructType implementation
 */
Type &DefineStructTypeInst::getType() const {
  return TypeAnalysis::get().get(this->getCallInst());
}

std::string DefineStructTypeInst::getName() const {
  auto name_value = this->getNameOperand();

  GlobalVariable *name_global;
  auto name_gep = dyn_cast<GetElementPtrInst>(name_value);
  if (name_gep) {
    auto name_ptr = name_gep->getPointerOperand();
    name_global = dyn_cast<GlobalVariable>(name_ptr);
  } else {
    name_global = dyn_cast<GlobalVariable>(name_value);
  }

  MEMOIR_NULL_CHECK(name_global, "DefineStructTypeInst has NULL name");

  auto name_init = name_global->getInitializer();
  auto name_constant = dyn_cast<ConstantDataArray>(name_init);
  MEMOIR_NULL_CHECK(name_constant,
                    "DefineStructTypeInst name is not a constant data array");

  return name_constant->getAsCString();
}

llvm::Value &DefineStructTypeInst::getNameOperand() const {
  return this->getNameOperandAsUse().get();
}

llvm::Use &DefineStructTypeInst::getNameOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

unsigned DefineStructTypeInst::getNumberOfFields() const {
  auto num_fields_value = this->getNumberOfFieldsOperand();

  auto num_fields_const = dyn_cast<llvm::ConstantInt>(&num_fields_value);
  MEMOIR_NULL_CHECK(num_fields_const,
                    "DefineStructTypeInst with a non-static number of fields");

  auto num_fields = num_fields_const->getZExtValue();
  MEMOIR_ASSERT(
      (num_fields <= std::numeric_limits<unsigned>::max()),
      "DefineStructTypeInst has more field than the max value of unsigned type");

  return num_fields;
}

llvm::Value &DefineStructTypeInst::getNumberOfFieldsOperand() const {
  return this->getNumberOfFieldsOperandAsUse().get();
}

llvm::Use &DefineStructTypeInst::getNumberOfFieldsOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(1);
}

Type &DefineStructTypeInst::getFieldType(unsigned field_index) const {
  return TypeAnalysis::get().getType(this->getFieldTypeOperand(field_index));
}

llvm::Value &DefineStructTypeInst::getFieldTypeOperand(
    unsigned field_index) const {
  return this->getFieldTypeOperandAsUse(field_index).get();
}

llvm::Use &DefineStructTypeInst::getFieldTypeOperandAsUse(
    unsigned field_index) const {
  return this->getCallInst().getArgOperandUse(2 + field_index);
}

std::string DefineStructTypeInst::toString(std::string indent = "") const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "DefineStructTypeInst: " + llvm_str;

  return str;
}

/*
 * StructType implementation
 */
Type &StructTypeInst::getType() const {
  return TypeAnalysis::get().getType(this->getCallInst());
}

std::string StructTypeInst::getName() const {
  auto name_value = this->getNameOperand();

  GlobalVariable *name_global;
  auto name_gep = dyn_cast<GetElementPtrInst>(name_value);
  if (name_gep) {
    auto name_ptr = name_gep->getPointerOperand();
    name_global = dyn_cast<GlobalVariable>(name_ptr);
  } else {
    name_global = dyn_cast<GlobalVariable>(name_value);
  }

  MEMOIR_NULL_CHECK(name_global, "DefineStructTypeInst has NULL name");

  auto name_init = name_global->getInitializer();
  auto name_constant = dyn_cast<ConstantDataArray>(name_init);
  MEMOIR_NULL_CHECK(name_constant,
                    "DefineStructTypeInst name is not a constant data array");

  return name_constant->getAsCString();
}

llvm::Value &StructTypeInst::getNameOperand() const {
  return this->getNameOperandAsUse().get();
}

llvm::Use &StructTypeInst::getNameOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

std::string StructTypeInst::toString(std::string indent = "") const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "StructTypeInst: " + llvm_str;

  return str;
}

/*
 * StaticTensorType implementation
 */
Type &StaticTensorTypeInst::getType() const {
  return TypeAnalysis::get().getType(this->getCallInst());
}

Type &StaticTensorTypeInst::getElementType() const {
  return TypeAnalysis::get().getType(this->getElementTypeOperand());
}

llvm::Value &StaticTensorTypeInst::getElementTypeOperand() const {
  return this->getElementTypeOperandAsUse().get();
}

llvm::Use &StaticTensorTypeInst::getElementTypeOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

unsigned StaticTensorTypeInst::getNumberOfDimensions() const {
  auto dims_value = this->getNumberOfDimensionsOperand();

  auto dims_const = dyn_cast<llvm::ConstantInt>(&dims_value);
  MEMOIR_NULL_CHECK(
      dims_const,
      "Number of dimensions passed to StaticTensorType is non-static");

  auto num_dims = dims_const->getZExtValue();
  MEMOIR_ASSERT(
      (num_dims <= std::numeric_limits<unsigned>::max()),
      "Number of dimensions is larger than maximum value of unsigned");

  return num_dims;
}

llvm::Value &StaticTensorTypeInst::getNumberOfDimensionsOperand() const {
  return this->getNumberOfDimensionsOperandAsUse().get();
}

llvm::Use &StaticTensorTypeInst::getNumberOfDimensionsOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(1);
}

size_t StaticTensorTypeInst::getLengthOfDimension(
    unsigned dimension_index) const {
  auto length_value = this->getLengthOfDimensionOperand(dimension_index);

  auto length_const = dyn_cast<llvm::ConstantInt>(&length_value);
  MEMOIR_NULL_CHECK(
      length_const,
      "Attempt to create a static tensor type of non-static length");

  auto length = length_const->getZExtValue();
  MEMOIR_ASSERT(
      (length < std::numeric_limits<size_t>::max()),
      "Attempt to create static tensor type larger than maximum value of size_t");

  return length;
}

llvm::Value &StaticTensorTypeInst::getLengthOfDimensionOperand(
    unsigned dimension_index) const {
  return this->getLengthOfDimensionOperandAsUse(dimension_index).get();
}

llvm::Use &StaticTensorTypeInst::getLengthOfDimensionOperandAsUse(
    unsigned dimension_index) const {
  return this->getCallInst().getArgOperandUse(2 + dimension_index);
}

std::string StaticTensorTypeInst::toString(std::string indent = "") const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "StaticTensorTypeInst: " + llvm_str;

  return str;
}

/*
 * TensorType implementation
 */
Type &TensorTypeInst::getType() const {
  return TypeAnalysis::get().getType(this->getCallInst());
}

Type &TensorTypeInst::getElementType() const {
  return TypeAnalysis::get().getType(this->getElementTypeOperand());
}

llvm::Value &TensorTypeInst::getElementTypeOperand() const {
  return this->getElementTypeOperandAsUse().get();
}

llvm::Use &TensorTypeInst::getElementTypeOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

unsigned TensorTypeInst::getNumberOfDimensions() const {
  auto dims_value = this->getNumberOfDimensionsOperand();

  auto dims_const = dyn_cast<llvm::ConstantInt>(&dims_value);
  MEMOIR_NULL_CHECK(dims_const,
                    "Number of dimensions passed to TensorType is non-static");

  auto num_dims = dims_const->getZExtValue();
  MEMOIR_ASSERT(
      (num_dims <= std::numeric_limits<unsigned>::max()),
      "Number of dimensions passed to TensorType is larger than maximum value of unsigned");

  return num_dims;
}

llvm::Value &TensorTypeInst::getNumberOfDimensionsOperand() const {
  return this->getNumberOfDimensionsOperandAsUse().get();
}

llvm::Use &TensorTypeInst::getNumberOfDimensionsOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(1);
}

std::string TensorTypeInst::toString(std::string indent = "") const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "TensorTypeInst: " + llvm_str;

  return str;
}

/*
 * AssocArrayType implementation
 */
Type &AssocArrayTypeInst::getType() const {
  return TypeAnalysis::get().getType(this->getCallInst());
}

Type &AssocArrayTypeInst::getKeyType() const {
  return TypeAnalysis::get().getType(this->getKeyOperand());
}

llvm::Value &AssocArrayTypeInst::getKeyOperand() const {
  return this->getKeyOperandAsUse().get();
}

llvm::Use &AssocArrayTypeInst::getKeyOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

Type &AssocArrayTypeInst::getValueType() const {
  return TypeAnalysis::get().getType(this->getValueOperand());
}

llvm::Value &AssocArrayTypeInst::getValueOperand() const {
  return this->getValueOperandAsUse().get();
}

llvm::Use &AssocArrayTypeInst::getValueOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(1);
}

std::string AssocArrayTypeInst::toString(std::string indent = "") const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "AssocArrayTypeInst: " + llvm_str;

  return str;
}

/*
 * SequenceType implementation
 */
Type &SequenceTypeInst::getType() const {
  return TypeAnalysis::get().getType(this->getCallInst());
}

Type &SequenceTypeInst::getElementType() const {
  return TypeAnalysis::get().getType(this->getElementOperand());
}

llvm::Value &SequenceTypeInst::getElementOperand() const {
  return this->getElementOperandAsUse().get();
}

llvm::Use &SequenceTypeInst::getElementOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

std::string SequenceTypeInst::toString(std::string indent = "") const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "SequenceTypeInst: " + llvm_str;

  return str;
}
