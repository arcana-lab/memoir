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

ObjectField::ObjectField(Type *type) : ObjectField(type, new Object(type)) {}

ObjectField::ObjectField(Type *type, Object *init) : Field(type), value(init) {}

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

void ObjectField::writeValue(Object *value) {
  this->value = value;
}

Object *ObjectField::readValue() {
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
  auto resolved_type = type->resolve();

  switch (resolved_type->getCode()) {
    case TypeCode::StructTy:
    case TypeCode::TensorTy:
      return new ObjectField(resolved_type);
    case TypeCode::IntegerTy:
      return new IntegerField(resolved_type);
    case TypeCode::FloatTy:
      return new FloatField(resolved_type);
    case TypeCode::DoubleTy:
      return new DoubleField(resolved_type);
    case TypeCode::ReferenceTy:
      return new ReferenceField(resolved_type);
    case TypeCode::StubTy:
      std::cerr << "ERROR: Stub Type not resolved before field construction\n";
      exit(1);
    default:
      std::cerr << "ERROR: Trying to create field of unknown type\n";
      exit(1);
  }
}

} // namespace memoir
