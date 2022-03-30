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

Type *Field::getType() {
  return this->type;
}

Field *Field::createField(Type *type) {
  switch (type->getCode()) {
    case TypeCode::ObjectTy:
    case TypeCode::ArrayTy:
    case TypeCode::UnionTy:
      std::cerr << "Object type found\n";
      return new ObjectField(type);
    case TypeCode::IntegerTy:
      std::cerr << "Integer type found\n";
      return new IntegerField(type);
    case TypeCode::FloatTy:
      std::cerr << "Float type found\n";
      return new FloatField(type);
    case TypeCode::DoubleTy:
      std::cerr << "Double type found\n";
      return new DoubleField(type);
    default:
      // Error
      break;
  }
}
