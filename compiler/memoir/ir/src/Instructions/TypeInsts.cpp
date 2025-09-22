#include <limits>

#include "memoir/ir/Instructions.hpp"

#include "memoir/ir/TypeCheck.hpp"

#include "memoir/utility/InstructionUtils.hpp"

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
    auto type = type_of(this->getCallInst());                                  \
    MEMOIR_NULL_CHECK(type, "Could not determine type of " #CLASS_NAME);       \
    return *type;                                                              \
  }

/*
 * UInt64TypeInst implementation
 */
GET_TYPE_IMPL(UInt64TypeInst)
TO_STRING(UInt64TypeInst, "u64")

/*
 * UInt32TypeInst implementation
 */
GET_TYPE_IMPL(UInt32TypeInst)
TO_STRING(UInt32TypeInst, "u32")

/*
 * UInt16TypeInst implementation
 */
GET_TYPE_IMPL(UInt16TypeInst)
TO_STRING(UInt16TypeInst, "u16")

/*
 * UInt8TypeInst implementation
 */
GET_TYPE_IMPL(UInt8TypeInst)
TO_STRING(UInt8TypeInst, "u8")

// UInt8TypeInst implementation
GET_TYPE_IMPL(UInt2TypeInst)
TO_STRING(UInt2TypeInst, "u2")

/*
 * Int64TypeInst implementation
 */
GET_TYPE_IMPL(Int64TypeInst)
TO_STRING(Int64TypeInst, "i64")

/*
 * Int32TypeInst implementation
 */
GET_TYPE_IMPL(Int32TypeInst)
TO_STRING(Int32TypeInst, "i32")

/*
 * Int16TypeInst implementation
 */
GET_TYPE_IMPL(Int16TypeInst)
TO_STRING(Int16TypeInst, "i16")

/*
 * Int8TypeInst implementation
 */
GET_TYPE_IMPL(Int8TypeInst)
TO_STRING(Int8TypeInst, "i8")

/*
 * Int2TypeInst implementation
 */
GET_TYPE_IMPL(Int2TypeInst)
TO_STRING(Int2TypeInst, "i2")

/*
 * BoolTypeInst implementation
 */
GET_TYPE_IMPL(BoolTypeInst)
TO_STRING(BoolTypeInst, "bool")

/*
 * FloatType implementation
 */
GET_TYPE_IMPL(FloatTypeInst)
TO_STRING(FloatTypeInst, "f32")

/*
 * DoubleType implementation
 */
GET_TYPE_IMPL(DoubleTypeInst)
TO_STRING(DoubleTypeInst, "f64")

/*
 * PointerType implementation
 */
GET_TYPE_IMPL(PointerTypeInst)
TO_STRING(PointerTypeInst, "ptr")

/*
 * PointerType implementation
 */
GET_TYPE_IMPL(VoidTypeInst)
TO_STRING(VoidTypeInst, "void")

/*
 * ReferenceType implementation
 */
GET_TYPE_IMPL(ReferenceTypeInst)

Type &ReferenceTypeInst::getReferencedType() const {
  auto type = type_of(this->getReferencedTypeOperand());
  MEMOIR_NULL_CHECK(
      type,
      "Could not determine the referenced type of ReferenceTypeInst");
  return *type;
}

OPERAND(ReferenceTypeInst, ReferencedTypeOperand, 0)

TO_STRING(ReferenceTypeInst, "ref")

#if 0
/*
 * DefineType implementation
 */
std::string DefineTypeInst::getName() const {
  auto &name_value = this->getNameOperand();

  GlobalVariable *name_global = nullptr;
  if (auto name_gep = dyn_cast<llvm::GetElementPtrInst>(&name_value)) {
    auto name_ptr = name_gep->getPointerOperand();
    name_global = dyn_cast<GlobalVariable>(name_ptr);
  } else if (auto name_const_gep = dyn_cast<llvm::ConstantExpr>(&name_value)) {
    auto name_ptr = name_const_gep->getOperand(0);
    name_global = dyn_cast<GlobalVariable>(name_ptr);
  } else {
    name_global = dyn_cast<GlobalVariable>(&name_value);
  }

  MEMOIR_NULL_CHECK(name_global, "DefineTypeInst has NULL name");

  auto name_init = name_global->getInitializer();
  auto name_constant = dyn_cast<ConstantDataArray>(name_init);
  MEMOIR_NULL_CHECK(name_constant,
                    "DefineTypeInst name is not a constant data array");

  return name_constant->getAsCString().str();
}

OPERAND(DefineTypeInst, NameOperand, 0)

Type &DefineTypeInst::getType() const {
  auto type = type_of(this->getTypeOperand());
  MEMOIR_NULL_CHECK(type, "Could not determine the type being defined");
  return *type;
}

OPERAND(DefineTypeInst, TypeOperand, 1)

TO_STRING(DefineTypeInst, "deftype")

/*
 * LookupType implementation
 */
GET_TYPE_IMPL(LookupTypeInst)

std::string LookupTypeInst::getName() const {
  auto &name_value = this->getNameOperand();

  GlobalVariable *name_global;
  auto name_gep = dyn_cast<GetElementPtrInst>(&name_value);
  if (name_gep) {
    auto name_ptr = name_gep->getPointerOperand();
    name_global = dyn_cast<GlobalVariable>(name_ptr);
  } else {
    name_global = dyn_cast<GlobalVariable>(&name_value);
  }

  MEMOIR_NULL_CHECK(name_global, "LookupTypeInst has NULL name");

  auto name_init = name_global->getInitializer();
  auto name_constant = dyn_cast<ConstantDataArray>(name_init);
  MEMOIR_NULL_CHECK(name_constant,
                    "DefineTypeInst name is not a constant data array");

  return name_constant->getAsCString().str();
}

OPERAND(LookupTypeInst, NameOperand, 0)

TO_STRING(LookupTypeInst, "lookuptype")

#endif

/*
 * TupleType implementation
 */
GET_TYPE_IMPL(TupleTypeInst)

unsigned TupleTypeInst::getNumberOfFields() const {
  return std::distance(this->getCallInst().arg_begin(),
                       this->kw_begin().asUse());
}

Type &TupleTypeInst::getFieldType(unsigned field_index) const {
  auto type = type_of(this->getFieldTypeOperand(field_index));
  MEMOIR_NULL_CHECK(type, "Could not determine field type of DefineTypeInst");
  return *type;
}

VAR_OPERAND(TupleTypeInst, FieldTypeOperand, 0)

TO_STRING(TupleTypeInst, "tuple")

/*
 * ArrayType implementation
 */
GET_TYPE_IMPL(ArrayTypeInst)

Type &ArrayTypeInst::getElementType() const {
  auto type = type_of(this->getElementTypeOperand());
  MEMOIR_NULL_CHECK(type, "Could not determine element type of ArrayTypeInst");
  return *type;
}

OPERAND(ArrayTypeInst, ElementTypeOperand, 0)

size_t ArrayTypeInst::getLength() const {
  auto &value = this->getLengthOperand();
  auto &constant = MEMOIR_SANITIZE(dyn_cast<llvm::ConstantInt>(&value),
                                   "ArrayType length is not statically known!");
  return constant.getZExtValue();
}

OPERAND(ArrayTypeInst, LengthOperand, 1)

TO_STRING(ArrayTypeInst, "array")

/*
 * AssocArrayType implementation
 */
GET_TYPE_IMPL(AssocArrayTypeInst)

Type &AssocArrayTypeInst::getKeyType() const {
  auto type = type_of(this->getKeyOperand());
  MEMOIR_NULL_CHECK(type, "Could not determine the key type of AssocArrayType");
  return *type;
}

OPERAND(AssocArrayTypeInst, KeyOperand, 0)

Type &AssocArrayTypeInst::getValueType() const {
  auto type = type_of(this->getValueOperand());
  MEMOIR_NULL_CHECK(type,
                    "Could not determine the value type of AssocArrayType");
  return *type;
}

OPERAND(AssocArrayTypeInst, ValueOperand, 1)

TO_STRING(AssocArrayTypeInst, "assoc")

/*
 * SequenceType implementation
 */
GET_TYPE_IMPL(SequenceTypeInst)

Type &SequenceTypeInst::getElementType() const {
  auto type = type_of(this->getElementOperand());
  MEMOIR_NULL_CHECK(type, "Could not determine element type of SequenceType");
  return *type;
}

OPERAND(SequenceTypeInst, ElementOperand, 0)

TO_STRING(SequenceTypeInst, "seq")

} // namespace llvm::memoir
