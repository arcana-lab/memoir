#include <iostream>

#include "objects.h"

using namespace objectir;

Type *Object::getType() {
  return this->type;
}

Object::Object(Type *ty) : type(ty) {
  // Resolve the type.
  TypeCode code = this->type->getCode();
  if (code == TypeCode::StubTy) {
    auto stubType = (StubType *)(this->type);
    auto resolvedType = stubType->resolve();
    this->type = resolvedType;
  }

  // Create object.
  switch (this->type->getCode()) {
    case TypeCode::ObjectTy: {
      // Initialize the fields
      auto objectType = (ObjectType *)(this->type);
      for (auto fieldType : objectType->fields) {
        auto field = Field::createField(fieldType);
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

Array::Array(Type *ty, uint64_t length)
  : Object(ty),
    length(length) {

  if (this->type->getCode() != TypeCode::ArrayTy) {
    std::cerr
        << "Trying to create an array of non-array type\n";
    exit(1);
  }

  Type *elementTy =
      ((ArrayType *)(this->type))->elementType;
  for (auto i = 0; i < length; i++) {
    auto field = Field::createField(elementTy);
    this->fields.push_back(field);
  }
}

Union::Union(Type *type) : Object(type) {
  // TODO
}

Object::~Object() {
  for (auto field : this->fields) {
    // delete field;
  }
}
