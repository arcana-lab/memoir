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

#define HANDLE_INTEGER_TYPE(TYPE_NAME, C_TYPE, BITWIDTH, IS_SIGNED)            \
  __RUNTIME_ATTR                                                               \
  Type *MEMOIR_FUNC(TYPE_NAME##_type)() {                                      \
    return IntegerType::get(BITWIDTH, IS_SIGNED);                              \
  }

#define HANDLE_PRIMITIVE_TYPE(TYPE_NAME, C_TYPE, CLASS_PREFIX)                 \
  __RUNTIME_ATTR                                                               \
  Type *MEMOIR_FUNC(TYPE_NAME##_type)() {                                      \
    return CLASS_PREFIX##Type::get();                                          \
  }
#include "types.def"

__RUNTIME_ATTR
Type *MEMOIR_FUNC(ref_type)(Type *referenced_type) {
  return ReferenceType::get(referenced_type);
}

__ALLOC_ATTR
__RUNTIME_ATTR
Struct *MEMOIR_FUNC(allocate_struct)(Type *type) {
  auto strct = new struct Struct(type);

  return strct;
}

__ALLOC_ATTR
__RUNTIME_ATTR
Collection *MEMOIR_FUNC(allocate_tensor)(Type *element_type,
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
Collection *MEMOIR_FUNC(allocate_assoc_array)(Type *key_type,
                                              Type *value_type) {
  auto assoc_array_type = AssocArrayType::get(key_type, value_type);

  auto assoc_array = new struct AssocArray(assoc_array_type);

  return assoc_array;
}

__ALLOC_ATTR
__RUNTIME_ATTR
Collection *MEMOIR_FUNC(allocate_sequence)(Type *element_type,
                                           uint64_t init_size) {
  auto sequence_type = SequenceType::get(element_type);

  auto sequence = new struct SequenceAlloc(sequence_type, init_size);

  return sequence;
}

__ALLOC_ATTR
__RUNTIME_ATTR
Collection *MEMOIR_FUNC(join)(uint8_t number_of_collections,
                              Collection *collection_to_join,
                              ...) {
  MEMOIR_ASSERT((collection_to_join != nullptr),
                "Attempt to join with NULL object.");

  va_list args;

  va_start(args, collection_to_join);

  auto joined_object =
      collection_to_join->join(args, number_of_collections - 1);

  va_end(args);

  return joined_object;
}

__ALLOC_ATTR
__RUNTIME_ATTR
Collection *MEMOIR_FUNC(get_slice)(Collection *collection_to_slice, ...) {
  MEMOIR_ASSERT((collection_to_slice != nullptr),
                "Attempt to slice NULL object");

  va_list args;

  va_start(args, collection_to_slice);

  auto sliced_object = collection_to_slice->get_slice(args);

  va_end(args);

  return sliced_object;
}

__RUNTIME_ATTR
Collection *MEMOIR_FUNC(view)(Collection *collection_to_view,
                              size_t begin,
                              size_t end) {
  MEMOIR_ASSERT((collection_to_view != nullptr), "Attempt to view NULL object");
  if (auto *seq = dynamic_cast<SequenceAlloc *>(collection_to_view)) {
    return new SequenceView(seq, begin, end);
  } else if (auto *seq_view =
                 dynamic_cast<SequenceView *>(collection_to_view)) {
    return new SequenceView(seq_view, begin, end);
  }
  MEMOIR_UNREACHABLE("Attempt to view a non-viewable object");
}

__RUNTIME_ATTR
void MEMOIR_FUNC(delete_struct)(Struct *strct) {
  delete strct;
}

__RUNTIME_ATTR
void MEMOIR_FUNC(delete_collection)(Collection *cllct) {
  delete cllct;
}

} // extern "C"
} // namespace memoir
