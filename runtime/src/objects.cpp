#include <iostream>

#include "objects.h"

namespace memoir {

Type *Object::getType() {
  return this->type;
}

Object::Object(Type *type) : type(type) {
  // Resolve the type.
  TypeCode code = this->type->getCode();
  if (code == TypeCode::StubTy) {
    auto stub_type = (StubType *)(this->type);
    auto resolved_type = stub_type->resolve();
    this->type = resolved_type;
  }
}

/*
 * Struct Objects
 */
Struct::Struct(Type *type) : Object(type) {
  if (this->type->getCode() != TypeCode::StructTy) {
    std::cerr << "Trying to create a struct of non-struct type\n";
    exit(1);
  }

  // Initialize the fields
  auto object_type = (StructType *)(this->type);
  for (auto field_type : object_type->fields) {
    auto field = Field::createField(field_type);
    this->fields.push_back(field);
  }
}

/*
 * Tensor Objects
 */
Tensor::Tensor(Type *type, std::vector<uint64_t> &length_of_dimensions)
  : Object(type),
    length_of_dimensions(length_of_dimensions) {

  if (this->type->getCode() != TypeCode::TensorTy) {
    std::cerr << "Trying to create a tensor of non-tensor type\n";
    exit(1);
  }

  // Initialize the tensor
  auto tensor_type = (TensorType *)(this->type);
  auto inner_type = tensor_type->element_type;
  auto num_dimensions = tensor_type->num_dimensions;
  if (num_dimensions > 1) {
    // This is a multi-dimensional tensor, build inner tensors
    // Clone the dimension lengths and pop off the front dimension
    std::vector<uint64_t> inner_dimensions(length_of_dimensions);
    inner_dimensions.erase(inner_dimensions.begin());

    // Build inner tensors
    auto dimension_length = length_of_dimensions[0];
    auto element_type = TensorType::get(inner_type, num_dimensions - 1);
    for (auto i = 0; i < dimension_length; i++) {
      auto inner_tensor = new Tensor(element_type, inner_dimensions);
      this->fields.push_back(inner_tensor);
    }
  } else if (num_dimensions == 0) {
    // This is a one-dimensional tensor, build inner fields
    auto dimension_length = length_of_dimensions[0];
    auto element_type = inner_type;
    for (auto i = 0; i < dimension_length; i++) {
      auto field = Field::createField(element_type);
      this->fields.push_back(field);
    }
  } else {
    std::cerr << "Trying to create a tensor with 0 dimensions";
    exit(1);
  }
}

Object *Tensor::getElement(uint64_t index) {
  auto first_length = length_of_dimensions[0];
  if (index >= first_length) {
    std::cerr << "Tensor::getElement: Index out of range for tensor access\n";
    exit(1);
  }

  return fields[index];
}

} // namespace memoir
