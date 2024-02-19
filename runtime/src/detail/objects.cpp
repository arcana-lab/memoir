#include <iostream>

#include "internal.h"
#include "objects.h"

namespace memoir {
namespace detail {

/*
 * Abstract Object implementation
 */

Type *Object::get_type() const {
  return this->type;
}

Object::Object(Type *type) : type(type) {
  // Do nothing.
}

bool Object::is_collection() const {
  return false;
}

bool Object::is_struct() const {
  return false;
}

bool Object::is_element() const {
  return false;
}

/*
 * Struct Objects
 */
Struct::Struct(Type *type) : Object(type) {
  MEMOIR_ASSERT((type->getCode() == TypeCode::StructTy),
                "Trying to create a struct of non-struct type");

  // Initialize the fields
  auto object_type = (StructType *)(type);
  this->fields.reserve(object_type->fields.size());
  for (auto field_type : object_type->fields) {
    auto field = init_element(field_type);
    this->fields.push_back(field);
  }
}

Struct::Struct(Struct *other) : Object(other->type) {
  // Clone the fields.
  auto type = static_cast<StructType *>(other->get_type());
  this->fields.reserve(other->fields.size());
  for (auto i = 0; i < other->fields.size(); i++) {
    auto other_field = other->fields[i];
    // TODO: perform a deep copy of the other field if its a pointer.
    this->fields.push_back(other_field);
  }
}

void Struct::free() {
  auto *struct_type = static_cast<StructType *>(this->get_type());
  auto field_index = 0;
  for (auto *field_type : struct_type->fields) {
    if (is_object_type(field_type)) {
      ((Object *)(this->fields[field_index]))->free();
    }
    field_index++;
  }
}

uint64_t Struct::get_field(uint64_t field_index) const {
  MEMOIR_ASSERT((field_index < this->fields.size()),
                "Trying to read field from index outside of struct's range");

  return this->fields.at(field_index);
}

void Struct::set_field(uint64_t value, uint64_t field_index) {
  MEMOIR_ASSERT((field_index < this->fields.size()),
                "Trying to write field from index outside of struct's range");

  this->fields[field_index] = value;
}

bool Struct::equals(const Object *other) const {
  return (this == other);
}

bool Struct::is_struct() const {
  return true;
}

} // namespace detail
} // namespace memoir
