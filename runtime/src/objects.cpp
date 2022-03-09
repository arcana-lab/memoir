#include "objects.hpp"

using namespace objectir;

Object::Object(Type *type) : type(type) {
  if (type->isObject) {
    // Initialize the fields
    auto objectType = (ObjectType *)type;
    for (auto fieldType : objectType->fields) {
      this->fields = Field::createField(fieldType);
    }
  } else if (type->isArray) {
    // Do nothing.
  } else {
    // ERROR: object is not of object type
  }
}

Array::Array(Type *type, uint64_t length)
  : Object(buildArrayType(type)),
    length(length) {

  for (auto i = 0; i < length; i++) {
    this->fields.push_back(nullptr)
  }
}

Array::Array(Type *type, uint64_t length, Field *init)
  : Object(buildArrayType(type)),
    length(length) {

  for (auto i = 0; i < length; i++) {
    this->fields.push_back(init)
  }
}
