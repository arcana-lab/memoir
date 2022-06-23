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
    auto stub_type = (StubType *)(this->type);
    auto resolved_type = stub_type->resolve();
    this->type = resolved_type;
  }

  // Create object.
  switch (this->type->getCode()) {
    case TypeCode::StructTy: {
      break;
    }
    case TypeCode::TensorTy:
      // Do nothing.
      break;
    default:
      // ERROR: object is not of object type
      break;
  }
}

Struct::Struct(Type *type) : Object(type) {
  if (this->type->getCode() != TypeCode::StructTy) {
    std::cerr << "Trying to create a struct of non-struct type\n";
    exit(1);
  }

  // Initialize the fields
  auto object_type = (StructType *)(this->type);
  for (auto field_type : objectType->fields) {
    auto field = Field::createField(field_type);
    this->fields.push_back(field);
  }
}

Tensor::Tensor(Type *ty, std::vector<uint64_t> &length_of_dimensions)
  : Object(ty),
    length_of_dimensions(length_of_dimensions) {

  if (this->type->getCode() != TypeCode::TensorTy) {
    std::cerr << "Trying to create an array of non-array type\n";
    exit(1);
  }

  Type *elementTy = ((TensorType *)(this->type))->elementType;
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
