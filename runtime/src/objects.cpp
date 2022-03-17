#include "objects.hpp"

using namespace objectir;

Object::Object(Type *type) : type(type) {
  switch (type->getCode()) {
    case TypeCode::ObjectTy: {
      // Initialize the fields
      auto objectType = (ObjectType *)type;
      for (auto fieldType : objectType->fields) {
        this->fields.push_back(
            Field::createField(fieldType));
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
  : Object(new ArrayType(type)),
    length(length) {

  for (auto i = 0; i < length; i++) {
    this->fields.push_back(nullptr);
  }
}
