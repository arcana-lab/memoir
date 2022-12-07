#include <iostream>
#include <stdarg.h>

#include "internal.h"
#include "memoir.h"

namespace memoir {

extern "C" {

/*
 * Type construction
 */
__RUNTIME_ATTR
Type *MEMOIR_FUNC(define_struct_type)(const char *name, int num_fields, ...) {
  std::vector<Type *> fields;

  va_list args;

  va_start(args, num_fields);

  for (int i = 0; i < num_fields; i++) {
    auto arg = va_arg(args, Type *);
    fields.push_back(arg);
  }

  va_end(args);

  auto type = StructType::define(name, fields);

  return type;
}

__RUNTIME_ATTR
Type *MEMOIR_FUNC(struct_type)(const char *name) {
  return StructType::get(name);
}

__RUNTIME_ATTR
Type *MEMOIR_FUNC(static_tensor_type)(Type *type,
                                      uint64_t num_dimensions,
                                      ...) {
  std::vector<uint64_t> length_of_dimensions;

  va_list args;

  va_start(args, num_dimensions);

  for (int i = 0; i < num_dimensions; i++) {
    auto arg = va_arg(args, uint64_t);
    length_of_dimensions.push_back(arg);
  }

  va_end(args);

  return TensorType::get(type, num_dimensions, length_of_dimensions);
}

__RUNTIME_ATTR
Type *MEMOIR_FUNC(tensor_type)(Type *element_type, uint64_t num_dimensions) {
  return TensorType::get(element_type, num_dimensions);
}

__RUNTIME_ATTR
Type *MEMOIR_FUNC(assoc_array_type)(Type *key_type, Type *value_type) {
  return AssocArrayType::get(key_type, value_type);
}

__RUNTIME_ATTR
Type *MEMOIR_FUNC(sequence_type)(Type *element_type) {
  return SequenceType::get(element_type);
}

__RUNTIME_ATTR
Type *MEMOIR_FUNC(u64_type)() {
  return IntegerType::get(64, false);
}

__RUNTIME_ATTR
Type *MEMOIR_FUNC(u32_type)() {
  return IntegerType::get(32, false);
}

__RUNTIME_ATTR
Type *MEMOIR_FUNC(u16_type)() {
  return IntegerType::get(16, false);
}

__RUNTIME_ATTR
Type *MEMOIR_FUNC(u8_type)() {
  return IntegerType::get(8, false);
}

__RUNTIME_ATTR
Type *MEMOIR_FUNC(i64_type)() {
  return IntegerType::get(64, true);
}

__RUNTIME_ATTR
Type *MEMOIR_FUNC(i32_type)() {
  return IntegerType::get(32, true);
}

__RUNTIME_ATTR
Type *MEMOIR_FUNC(i16_type)() {
  return IntegerType::get(16, true);
}

__RUNTIME_ATTR
Type *MEMOIR_FUNC(i8_type)() {
  return IntegerType::get(8, true);
}

__RUNTIME_ATTR
Type *MEMOIR_FUNC(bool_type)() {
  return IntegerType::get(1, false);
}

__RUNTIME_ATTR
Type *MEMOIR_FUNC(f32_type)() {
  return FloatType::get();
}

__RUNTIME_ATTR
Type *MEMOIR_FUNC(f64_type)() {
  return DoubleType::get();
}

__RUNTIME_ATTR
Type *MEMOIR_FUNC(ptr_type)() {
  return PointerType::get();
}

__RUNTIME_ATTR
Type *MEMOIR_FUNC(ref_type)(Type *referenced_type) {
  return ReferenceType::get(referenced_type);
}

__ALLOC_ATTR
__RUNTIME_ATTR
Object *MEMOIR_FUNC(allocate_struct)(Type *type) {
  auto strct = new struct Struct(type);

  return strct;
}

__ALLOC_ATTR
__RUNTIME_ATTR
Object *MEMOIR_FUNC(allocate_tensor)(Type *element_type,
                                     uint64_t num_dimensions,
                                     ...) {
  std::vector<uint64_t> length_of_dimensions;

  va_list args;

  va_start(args, num_dimensions);

  for (int i = 0; i < num_dimensions; i++) {
    auto length_of_dimension = va_arg(args, uint64_t);
    length_of_dimensions.push_back(length_of_dimension);
  }

  va_end(args);

  auto tensor_type = TensorType::get(element_type, num_dimensions);

  auto tensor = new struct Tensor(tensor_type, length_of_dimensions);

  return tensor;
}

__ALLOC_ATTR
__RUNTIME_ATTR
Object *MEMOIR_FUNC(allocate_assoc_array)(Type *key_type, Type *value_type) {
  auto assoc_array_type = AssocArrayType::get(key_type, value_type);

  auto assoc_array = new struct AssocArray(assoc_array_type);

  return assoc_array;
}

__ALLOC_ATTR
__RUNTIME_ATTR
Object *MEMOIR_FUNC(allocate_sequence)(Type *element_type, uint64_t init_size) {
  auto sequence_type = SequenceType::get(element_type);

  auto sequence = new struct Sequence(sequence_type, init_size);

  return sequence;
}

__RUNTIME_ATTR
Object *MEMOIR_FUNC(join)(uint8_t number_of_objects,
                          Object *object_to_join,
                          ...) {
  MEMOIR_ASSERT((object_to_join != nullptr),
                "Attempt to join with NULL object.");

  va_list args;

  va_start(args, object_to_join);

  auto joined_object = object_to_join->join(args, number_of_objects - 1);

  va_end(args);

  return joined_object;
}

__RUNTIME_ATTR
void MEMOIR_FUNC(delete_object)(Object *obj) {
  delete obj;
}

} // extern "C"
} // namespace memoir
