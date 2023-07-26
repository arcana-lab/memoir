#include <limits>

#include "memoir/ir/Instructions.hpp"

#include "memoir/analysis/TypeAnalysis.hpp"

/*
 * Implementation of the MemOIR Type Instructions.
 *
 * Author(s): Tommy McMichen
 * Created: December 13, 2022
 */

namespace llvm::memoir {

/*
 * Common function implementations
 */
#define GET_TYPE_IMPL(CLASS_NAME)                                              \
  Type &CLASS_NAME::getType() const {                                          \
    auto type = TypeAnalysis::get().getType(this->getCallInst());              \
    MEMOIR_NULL_CHECK(type, "Could not determine type of " #CLASS_NAME);       \
    return *type;                                                              \
  }

#define TO_STRING_IMPL(CLASS_NAME)                                             \
  std::string CLASS_NAME::toString(std::string indent) const {                 \
    std::string str, llvm_str;                                                 \
    llvm::raw_string_ostream llvm_ss(llvm_str);                                \
    llvm_ss << this->getCallInst();                                            \
    str = #CLASS_NAME ": " + llvm_str;                                         \
    return str;                                                                \
  }

/*
 * UInt64TypeInst implementation
 */
GET_TYPE_IMPL(UInt64TypeInst)
TO_STRING_IMPL(UInt64TypeInst)

/*
 * UInt32TypeInst implementation
 */
GET_TYPE_IMPL(UInt32TypeInst)
TO_STRING_IMPL(UInt32TypeInst)

/*
 * UInt16TypeInst implementation
 */
GET_TYPE_IMPL(UInt16TypeInst)
TO_STRING_IMPL(UInt16TypeInst)

/*
 * UInt8TypeInst implementation
 */
GET_TYPE_IMPL(UInt8TypeInst)
TO_STRING_IMPL(UInt8TypeInst)

/*
 * Int64TypeInst implementation
 */
GET_TYPE_IMPL(Int64TypeInst)
TO_STRING_IMPL(Int64TypeInst)

/*
 * Int32TypeInst implementation
 */
GET_TYPE_IMPL(Int32TypeInst)
TO_STRING_IMPL(Int32TypeInst)

/*
 * Int16TypeInst implementation
 */
GET_TYPE_IMPL(Int16TypeInst)
TO_STRING_IMPL(Int16TypeInst)

/*
 * Int8TypeInst implementation
 */
GET_TYPE_IMPL(Int8TypeInst)
TO_STRING_IMPL(Int8TypeInst)

/*
 * Int2TypeInst implementation
 */
GET_TYPE_IMPL(Int2TypeInst)
TO_STRING_IMPL(Int2TypeInst)

/*
 * BoolTypeInst implementation
 */
GET_TYPE_IMPL(BoolTypeInst)
TO_STRING_IMPL(BoolTypeInst)

/*
 * FloatType implementation
 */
GET_TYPE_IMPL(FloatTypeInst)
TO_STRING_IMPL(FloatTypeInst)

/*
 * DoubleType implementation
 */
GET_TYPE_IMPL(DoubleTypeInst)
TO_STRING_IMPL(DoubleTypeInst)

/*
 * PointerType implementation
 */
GET_TYPE_IMPL(PointerTypeInst)
TO_STRING_IMPL(PointerTypeInst)

/*
 * ReferenceType implementation
 */
GET_TYPE_IMPL(ReferenceTypeInst)

Type &ReferenceTypeInst::getReferencedType() const {
  auto type = TypeAnalysis::analyze(this->getReferencedTypeOperand());
  MEMOIR_NULL_CHECK(
      type,
      "Could not determine the referenced type of ReferenceTypeInst");
  return *type;
}

llvm::Value &ReferenceTypeInst::getReferencedTypeOperand() const {
  return *(this->getReferencedTypeAsUse().get());
}

llvm::Use &ReferenceTypeInst::getReferencedTypeAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

TO_STRING_IMPL(ReferenceTypeInst)

/*
 * DefineStructType implementation
 */
GET_TYPE_IMPL(DefineStructTypeInst)

std::string DefineStructTypeInst::getName() const {
  auto &name_value = this->getNameOperand();

  GlobalVariable *name_global;
  auto name_gep = dyn_cast<GetElementPtrInst>(&name_value);
  if (name_gep) {
    auto name_ptr = name_gep->getPointerOperand();
    name_global = dyn_cast<GlobalVariable>(name_ptr);
  } else {
    name_global = dyn_cast<GlobalVariable>(&name_value);
  }

  MEMOIR_NULL_CHECK(name_global, "DefineStructTypeInst has NULL name");

  auto name_init = name_global->getInitializer();
  auto name_constant = dyn_cast<ConstantDataArray>(name_init);
  MEMOIR_NULL_CHECK(name_constant,
                    "DefineStructTypeInst name is not a constant data array");

  return name_constant->getAsCString();
}

llvm::Value &DefineStructTypeInst::getNameOperand() const {
  return *(this->getNameOperandAsUse().get());
}

llvm::Use &DefineStructTypeInst::getNameOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

unsigned DefineStructTypeInst::getNumberOfFields() const {
  auto &num_fields_value = this->getNumberOfFieldsOperand();

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
  return *(this->getNumberOfFieldsOperandAsUse().get());
}

llvm::Use &DefineStructTypeInst::getNumberOfFieldsOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(1);
}

Type &DefineStructTypeInst::getFieldType(unsigned field_index) const {
  auto type = TypeAnalysis::analyze(this->getFieldTypeOperand(field_index));
  MEMOIR_NULL_CHECK(type,
                    "Could not determine field type of DefineStructTypeInst");
  return *type;
}

llvm::Value &DefineStructTypeInst::getFieldTypeOperand(
    unsigned field_index) const {
  return *(this->getFieldTypeOperandAsUse(field_index).get());
}

llvm::Use &DefineStructTypeInst::getFieldTypeOperandAsUse(
    unsigned field_index) const {
  return this->getCallInst().getArgOperandUse(2 + field_index);
}

TO_STRING_IMPL(DefineStructTypeInst)

/*
 * StructType implementation
 */
GET_TYPE_IMPL(StructTypeInst)

std::string StructTypeInst::getName() const {
  auto &name_value = this->getNameOperand();

  GlobalVariable *name_global;
  auto name_gep = dyn_cast<GetElementPtrInst>(&name_value);
  if (name_gep) {
    auto name_ptr = name_gep->getPointerOperand();
    name_global = dyn_cast<GlobalVariable>(name_ptr);
  } else {
    name_global = dyn_cast<GlobalVariable>(&name_value);
  }

  MEMOIR_NULL_CHECK(name_global, "DefineStructTypeInst has NULL name");

  auto name_init = name_global->getInitializer();
  auto name_constant = dyn_cast<ConstantDataArray>(name_init);
  MEMOIR_NULL_CHECK(name_constant,
                    "DefineStructTypeInst name is not a constant data array");

  return name_constant->getAsCString();
}

llvm::Value &StructTypeInst::getNameOperand() const {
  return *(this->getNameOperandAsUse().get());
}

llvm::Use &StructTypeInst::getNameOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

TO_STRING_IMPL(StructTypeInst)

/*
 * StaticTensorType implementation
 */
GET_TYPE_IMPL(StaticTensorTypeInst)

Type &StaticTensorTypeInst::getElementType() const {
  auto type = TypeAnalysis::analyze(this->getElementTypeOperand());
  MEMOIR_NULL_CHECK(type,
                    "Could not determine element type of StaticTensorTypeInst");
  return *type;
}

llvm::Value &StaticTensorTypeInst::getElementTypeOperand() const {
  return *(this->getElementTypeOperandAsUse().get());
}

llvm::Use &StaticTensorTypeInst::getElementTypeOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

unsigned StaticTensorTypeInst::getNumberOfDimensions() const {
  auto &dims_value = this->getNumberOfDimensionsOperand();

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
  return *(this->getNumberOfDimensionsOperandAsUse().get());
}

llvm::Use &StaticTensorTypeInst::getNumberOfDimensionsOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(1);
}

size_t StaticTensorTypeInst::getLengthOfDimension(
    unsigned dimension_index) const {
  auto &length_value = this->getLengthOfDimensionOperand(dimension_index);

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
  return *(this->getLengthOfDimensionOperandAsUse(dimension_index).get());
}

llvm::Use &StaticTensorTypeInst::getLengthOfDimensionOperandAsUse(
    unsigned dimension_index) const {
  return this->getCallInst().getArgOperandUse(2 + dimension_index);
}

TO_STRING_IMPL(StaticTensorTypeInst)

/*
 * TensorType implementation
 */
GET_TYPE_IMPL(TensorTypeInst)

Type &TensorTypeInst::getElementType() const {
  auto type = TypeAnalysis::analyze(this->getElementOperand());
  MEMOIR_NULL_CHECK(type, "Could not determine the element type of TensorType");
  return *type;
}

llvm::Value &TensorTypeInst::getElementOperand() const {
  return *(this->getElementOperandAsUse().get());
}

llvm::Use &TensorTypeInst::getElementOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

unsigned TensorTypeInst::getNumberOfDimensions() const {
  auto &dims_value = this->getNumberOfDimensionsOperand();

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
  return *(this->getNumberOfDimensionsOperandAsUse().get());
}

llvm::Use &TensorTypeInst::getNumberOfDimensionsOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(1);
}

TO_STRING_IMPL(TensorTypeInst);

/*
 * AssocArrayType implementation
 */
GET_TYPE_IMPL(AssocArrayTypeInst)

Type &AssocArrayTypeInst::getKeyType() const {
  auto type = TypeAnalysis::analyze(this->getKeyOperand());
  MEMOIR_NULL_CHECK(type, "Could not determine the key type of AssocArrayType");
  return *type;
}

llvm::Value &AssocArrayTypeInst::getKeyOperand() const {
  return *(this->getKeyOperandAsUse().get());
}

llvm::Use &AssocArrayTypeInst::getKeyOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

Type &AssocArrayTypeInst::getValueType() const {
  auto type = TypeAnalysis::analyze(this->getValueOperand());
  MEMOIR_NULL_CHECK(type,
                    "Could not determine the value type of AssocArrayType");
  return *type;
}

llvm::Value &AssocArrayTypeInst::getValueOperand() const {
  return *(this->getValueOperandAsUse().get());
}

llvm::Use &AssocArrayTypeInst::getValueOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(1);
}

TO_STRING_IMPL(AssocArrayTypeInst)

/*
 * SequenceType implementation
 */
GET_TYPE_IMPL(SequenceTypeInst)

Type &SequenceTypeInst::getElementType() const {
  auto type = TypeAnalysis::analyze(this->getElementOperand());
  MEMOIR_NULL_CHECK(type, "Could not determine element type of SequenceType");
  return *type;
}

llvm::Value &SequenceTypeInst::getElementOperand() const {
  return *(this->getElementOperandAsUse().get());
}

llvm::Use &SequenceTypeInst::getElementOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

TO_STRING_IMPL(SequenceTypeInst)

} // namespace llvm::memoir
