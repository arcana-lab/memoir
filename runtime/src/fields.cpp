#include <iostream>

#include "objects.h"

namespace memoir {

/*
 * Field Constructors
 */
Field::Field(Type *type) : Object(type) {}

IntegerField::IntegerField(Type *type) : IntegerField(type, 0) {}

IntegerField::IntegerField(Type *type, uint64_t init)
  : Field(type),
    value(init) {}

FloatField::FloatField(Type *type) : FloatField(type, 0.0) {}

FloatField::FloatField(Type *type, float init) : Field(type), value(init) {}

DoubleField::DoubleField(Type *type) : DoubleField(type, 0.0) {}

DoubleField::DoubleField(Type *type, double init) : Field(type), value(init) {}

StructField::StructField(Type *type) : StructField(type, new Struct(type)) {}

StructField::StructField(Type *type, Struct *init) : Field(type), value(init) {}

TensorField::TensorField(Type *type) : TensorField(type, new Tensor(type)) {}

TensorField::TensorField(Type *type, Tensor *init) : Field(type), value(init) {}

ReferenceField::ReferenceField(Type *type) : ReferenceField(type, nullptr) {}

ReferenceField::ReferenceField(Type *type, Object *init)
  : Field(type),
    value(init) {}

/*
 * Field Accessors
 */
void IntegerField::writeValue(uint64_t value) {
  this->value = value;
}

uint64_t IntegerField::readValue() {
  return this->value;
}

void FloatField::writeValue(float value) {
  this->value = value;
}

float FloatField::readValue() {
  return this->value;
}

void DoubleField::writeValue(double value) {
  this->value = value;
}

double DoubleField::readValue() {
  return this->value;
}

Struct *StructField::readValue() {
  return this->value;
}

Tensor *TensorField::readValue() {
  return this->value;
}

void ReferenceField::writeValue(Object *value) {
  this->value = value;
}

Object *ReferenceField::readValue() {
  return this->value;
}

/*
 * Field factory method
 */

Field *Field::createField(Type *type) {
  switch (type->getCode()) {
    case TypeCode::StructTy:
      return new StructField(type);
    case TypeCode::TensorTy: {
      auto tensor_type = (TensorType *)type;
      if (!tensor_type->is_static_length) {
        std::cerr << "ERROR: Field of tensor type is not of static length\n";
        exit(1);
      }
      auto &length_of_dimensions = tensor_type->length_of_dimensions;
      auto tensor = new Tensor(type, length_of_dimensions);
      return new TensorField(type, tensor);
    }
    case TypeCode::IntegerTy:
      return new IntegerField(type);
    case TypeCode::FloatTy:
      return new FloatField(type);
    case TypeCode::DoubleTy:
      return new DoubleField(type);
    case TypeCode::ReferenceTy:
      return new ReferenceField(type);
    default:
      std::cerr << "ERROR: Trying to create field of unknown type\n";
      exit(1);
  }
}

} // namespace memoir
