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

Field *Struct::readField(uint64_t field_index) {
  if (field_index >= this->fields.size()) {
    std::cerr << "Trying to read field from index outside of struct's range\n";
    exit(1);
  }

  return this->fields.at(field_index);
}

/*
 * Tensor Objects
 */
Tensor::Tensor(Type *type) : Object(type) {

  if (this->type->getCode() != TypeCode::TensorTy) {
    std::cerr << "Trying to create a tensor of non-tensor type\n";
    exit(1);
  }

  auto tensor_type = (TensorType *)(this->type);
  auto element_type = tensor_type->element_type;
  auto num_dimensions = tensor_type->num_dimensions;
  this->length_of_dimensions = tensor_type->length_of_dimensions;

  if (num_dimensions < 1) {
    std::cerr << "Trying to create a tensor with 0 dimensions";
    exit(1);
  }

  // Initialize the tensor
  for (auto dimension_length : this->length_of_dimensions) {
    for (auto i = 0; i < dimension_length; i++) {
      auto field = Field::createField(element_type);
      this->fields.push_back(field);
    }
  }
}

Tensor::Tensor(Type *type, std::vector<uint64_t> &length_of_dimensions)
  : Object(type),
    length_of_dimensions(length_of_dimensions) {

  if (this->type->getCode() != TypeCode::TensorTy) {
    std::cerr << "Trying to create a tensor of non-tensor type\n";
    exit(1);
  }

  auto tensor_type = (TensorType *)(this->type);
  auto inner_type = tensor_type->element_type;
  auto num_dimensions = tensor_type->num_dimensions;

  if (num_dimensions < 1) {
    std::cerr << "Trying to create a tensor with 0 dimensions";
    exit(1);
  }
  // Initialize the tensor
  for (auto dim_idx = 0; dim_idx < num_dimensions; dim_idx++) {
    auto dimension_length = length_of_dimensions[0];
    auto element_type = inner_type;
    for (auto i = 0; i < dimension_length; i++) {
      auto field = Field::createField(element_type);
      this->fields.push_back(field);
    }
  }
}

Field *Tensor::getElement(std::vector<uint64_t> &indices) {
  auto flattened_index = 0;
  auto last_dimension_length = 1;
  for (auto i = 0; i < length_of_dimensions.size(); i++) {
    auto dimension_length = length_of_dimensions[i];
    auto index = indices[i];
    if (index >= dimension_length) {
      std::cerr << "Tensor::getElement: Index out of range for tensor access\n";
      exit(1);
    }

    flattened_index *= last_dimension_length;
    flattened_index += index;
    last_dimension_length = dimension_length;
  }

  return this->fields[flattened_index];
}

} // namespace memoir
