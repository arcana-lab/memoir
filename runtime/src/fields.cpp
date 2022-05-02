#include <iostream>

#include "objects.h"

using namespace objectir;

Field::Field(Type *type) : type(type) {}

IntegerField::IntegerField(Type *type)
  : Field(type),
    value(0) {}

IntegerField::IntegerField(Type *type, uint64_t init)
  : Field(type),
    value(init) {}

IntegerField::IntegerField(uint64_t init,
                           uint64_t bitwidth,
                           bool isSigned)
  : Field(new IntegerType(bitwidth, isSigned)),
    value(init) {}

FloatField::FloatField(Type *type)
  : Field(type),
    value(0.0) {}

FloatField::FloatField(Type *type, float init)
  : Field(type),
    value(init) {}

DoubleField::DoubleField(Type *type)
  : Field(type),
    value(0.0) {}

DoubleField::DoubleField(Type *type, double init)
  : Field(type),
    value(init) {}

ObjectField::ObjectField(Type *type)
  : Field(type),
    value(nullptr) {}

ObjectField::ObjectField(Object *obj)
  : Field(obj->getType()),
    value(obj) {}

PointerField::PointerField(Type *type)
  : Field(type),
    value(nullptr) {}

void PointerField::writeField(Object *value) {
  this->value = value;
}

Object *PointerField::readField() {
  return this->value;
}

Type *Field::getType() {
  return this->type;
}

Field *Field::createField(Type *type) {
  auto resolvedType = type->resolve();

  switch (resolvedType->getCode()) {
    case TypeCode::ObjectTy:
    case TypeCode::ArrayTy:
    case TypeCode::UnionTy:
      return new ObjectField(resolvedType);
    case TypeCode::IntegerTy:
      return new IntegerField(resolvedType);
    case TypeCode::FloatTy:
      return new FloatField(resolvedType);
    case TypeCode::DoubleTy:
      return new DoubleField(resolvedType);
    case TypeCode::PointerTy:
      return new PointerField(resolvedType);
    case TypeCode::StubTy:
      std::cerr
          << "ERROR: Stub Type not resolved before field construction\n";
      exit(1);
    default:
      std::cerr
          << "ERROR: Trying to create field of unknown type\n";
      exit(1);
  }
}
