#include <iostream>

#include "internal.h"
#include "objects.h"

namespace memoir {

/*
 * Element Constructors
 */
Element::Element(Type *type) : Object(type) {
  // Do nothing.
}

IntegerElement::IntegerElement(IntegerType *type) : IntegerElement(type, 0) {
  // Do nothing.
}

IntegerElement::IntegerElement(IntegerType *type, uint64_t init)
  : Element(type),
    value(init) {
  // Do nothing.
}

FloatElement::FloatElement(FloatType *type) : FloatElement(type, 0.0) {
  // Do nothing.
}

FloatElement::FloatElement(FloatType *type, float init)
  : Element(type),
    value(init) {
  // Do nothing.
}

DoubleElement::DoubleElement(DoubleType *type) : DoubleElement(type, 0.0) {
  // Do nothing.
}

DoubleElement::DoubleElement(DoubleType *type, double init)
  : Element(type),
    value(init) {
  // Do nothing.
}

ReferenceElement::ReferenceElement(ReferenceType *type)
  : ReferenceElement(type, nullptr) {
  // Do nothing.
}

ReferenceElement::ReferenceElement(ReferenceType *type, Object *init)
  : Element(type),
    value(init) {
  // Do nothing.
}

PointerElement::PointerElement(PointerType *type)
  : PointerElement(type, nullptr) {
  // Do nothing.
}

PointerElement::PointerElement(PointerType *type, void *init)
  : Element(type),
    value(init) {
  // Do nothing.
}

ObjectElement::ObjectElement(Type *type) : Element(type) {
  // Do nothing.
}

StructElement::StructElement(StructType *type)
  : StructElement(type, new Struct(type)) {
  // Do nothing.
}

StructElement::StructElement(StructType *type, Struct *init)
  : ObjectElement(type),
    value(init) {
  // Do nothing.
}

TensorElement::TensorElement(TensorType *type)
  : TensorElement(type, new Tensor(type)) {
  // Do nothing.
}

TensorElement::TensorElement(TensorType *type, Tensor *init)
  : ObjectElement(type),
    value(init) {
  // Do nothing.
}

/*
 * Element Accessors
 */
void IntegerElement::write_value(uint64_t value) {
  this->value = value;
}

uint64_t IntegerElement::read_value() const {
  return this->value;
}

void FloatElement::write_value(float value) {
  this->value = value;
}

float FloatElement::read_value() const {
  return this->value;
}

void DoubleElement::write_value(double value) {
  this->value = value;
}

double DoubleElement::read_value() const {
  return this->value;
}

void ReferenceElement::write_value(Object *value) {
  this->value = value;
}

Object *ReferenceElement::read_value() const {
  return this->value;
}

void PointerElement::write_value(void *value) {
  this->value = value;
}

void *PointerElement::read_value() const {
  return this->value;
}

Object *StructElement::read_value() const {
  return this->value;
}

Object *TensorElement::read_value() const {
  return this->value;
}

/*
 * Element cloning
 */
Element *IntegerElement::clone() const {
  auto type = static_cast<IntegerType *>(this->get_type());
  return new IntegerElement(type, this->value);
}

Element *FloatElement::clone() const {
  auto type = static_cast<FloatType *>(this->get_type());
  return new FloatElement(type, this->value);
}

Element *DoubleElement::clone() const {
  auto type = static_cast<DoubleType *>(this->get_type());
  return new DoubleElement(type, this->value);
}

Element *PointerElement::clone() const {
  auto type = static_cast<PointerType *>(this->get_type());
  return new PointerElement(type, this->value);
}

Element *ReferenceElement::clone() const {
  auto type = static_cast<ReferenceType *>(this->get_type());
  return new ReferenceElement(type, this->value);
}

Element *StructElement::clone() const {
  auto type = static_cast<StructType *>(this->get_type());
  auto cloned_struct = new Struct(type);
  for (auto i = 0; i < this->value->fields.size(); i++) {
    auto this_field = this->value->fields[i];
    cloned_struct->fields[i] = this_field->clone();
  }
  return new StructElement(type, cloned_struct);
}

Element *TensorElement::clone() const {
  auto type = static_cast<TensorType *>(this->get_type());
  auto cloned_tensor = new Tensor(type);
  for (auto i = 0; i < this->value->tensor.size(); i++) {
    auto this_element = this->value->tensor[i];
    cloned_tensor->tensor[i] = this_element->clone();
  }
  return new TensorElement(type, cloned_tensor);
}

/*
 * Element equality
 */
bool IntegerElement::equals(const Object *other) const {
  MEMOIR_TYPE_CHECK(other, TypeCode::IntegerTy);
  auto other_element = static_cast<const IntegerElement *>(other);
  return (this->read_value() == other_element->read_value());
}

bool FloatElement::equals(const Object *other) const {
  MEMOIR_TYPE_CHECK(other, TypeCode::FloatTy);
  auto other_element = static_cast<const FloatElement *>(other);
  return (this->read_value() == other_element->read_value());
}

bool DoubleElement::equals(const Object *other) const {
  MEMOIR_TYPE_CHECK(other, TypeCode::DoubleTy);
  auto other_element = static_cast<const DoubleElement *>(other);
  return (this->read_value() == other_element->read_value());
}

bool PointerElement::equals(const Object *other) const {
  MEMOIR_TYPE_CHECK(other, TypeCode::PointerTy);
  auto other_element = static_cast<const PointerElement *>(other);
  return (this->read_value() == other_element->read_value());
}

bool ReferenceElement::equals(const Object *other) const {
  MEMOIR_TYPE_CHECK(other, TypeCode::ReferenceTy);
  auto other_element = static_cast<const ReferenceElement *>(other);
  return (this->read_value() == other_element->read_value());
}

bool ObjectElement::equals(const Object *other) const {
  return (this == other);
}

/*
 * Element operations
 */
Object *Element::join(va_list args, uint8_t num_args) {
  MEMOIR_ASSERT(false,
                "Attempt to perform join operation on element, UNSUPPORTED");

  return nullptr;
}

Element *Element::get_element(va_list args) {
  return this;
}

Object *Element::get_slice(va_list args, uint8_t num_args) {
  MEMOIR_ASSERT(false,
                "Attempt to perform slice operation on element, UNSUPPORTED");

  return nullptr;
}

/*
 * Element factory method
 */
Element *Element::create(Type *type) {
  switch (type->getCode()) {
    case TypeCode::StructTy: {
      auto struct_type = static_cast<StructType *>(type);
      return new StructElement(struct_type);
    }
    case TypeCode::TensorTy: {
      auto tensor_type = (TensorType *)type;
      MEMOIR_ASSERT((tensor_type->is_static_length),
                    "Attempt to create tensor element of non-static length");
      auto &length_of_dimensions = tensor_type->length_of_dimensions;
      auto tensor = new Tensor(tensor_type, length_of_dimensions);
      return new TensorElement(tensor_type, tensor);
    }
    case TypeCode::IntegerTy: {
      auto integer_type = static_cast<IntegerType *>(type);
      return new IntegerElement(integer_type);
    }
    case TypeCode::FloatTy: {
      auto float_type = static_cast<FloatType *>(type);
      return new FloatElement(float_type);
    }
    case TypeCode::DoubleTy: {
      auto double_type = static_cast<DoubleType *>(type);
      return new DoubleElement(double_type);
    }
    case TypeCode::PointerTy: {
      auto ptr_type = static_cast<PointerType *>(type);
      return new PointerElement(ptr_type);
    }
    case TypeCode::ReferenceTy: {
      auto ref_type = static_cast<ReferenceType *>(type);
      return new ReferenceElement(ref_type);
    }
    default: {
      MEMOIR_ASSERT(false, "Attempting to create an element of unknown type");
    }
  }
}

} // namespace memoir
