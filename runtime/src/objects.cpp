#include <iostream>

#include "objects.h"

using namespace objectir;

Type *Object::getType() {
  return this->type;
}

Object::Object(Type *type) : type(type) {
  switch (type->getCode()) {
    case TypeCode::ObjectTy: {
      // Initialize the fields
      auto objectType = (ObjectType *)type;
      for (auto fieldType : objectType->fields) {
        std::cerr << fieldType->toString() << "\n";
        auto field = Field::createField(fieldType);
        std::cerr << field->toString() << "\n";
        this->fields.push_back(field);
      }
      break;
    }
    case TypeCode::ArrayTy:
      // Do nothing.
      break;
    default:
      // ERROR: object is not of object type
      break;
  }
}

Array::Array(Type *type, uint64_t length)
  : Object(type),
    length(length) {

  if (type->getCode() != TypeCode::ArrayTy) {
    std::cerr
        << "Trying to create an array of non-array type\n";
    exit(1);
  }

  Type *elementTy = ((ArrayType *)type)->elementType;
  for (auto i = 0; i < length; i++) {
    auto field = Field::createField(elementTy);
    this->fields.push_back(field);
  }
}

Union::Union(Type *type) : Object(type) {
  // TODO
}

Object::~Object() {
  delete this->type;
  for (auto field : this->fields) {
    delete field;
  }
}
