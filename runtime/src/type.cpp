#include "internal.h"
#include "memoir.h"
#include "types.h"

namespace memoir {

extern "C" {

// User-defined types.
__RUNTIME_ATTR
type_ref MEMOIR_FUNC(define_struct_type)(const char *name,
                                         int num_fields,
                                         ...) {
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
type_ref MEMOIR_FUNC(struct_type)(const char *name) {
  return StructType::get(name);
}

// Collection types.
__RUNTIME_ATTR
type_ref MEMOIR_FUNC(static_tensor_type)(type_ref type,
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

  return SequenceType::get(type);
}

__RUNTIME_ATTR
type_ref MEMOIR_FUNC(assoc_array_type)(const type_ref key_type,
                                       const type_ref value_type) {
  return AssocArrayType::get(key_type, value_type);
}

__RUNTIME_ATTR
type_ref MEMOIR_FUNC(sequence_type)(const type_ref element_type) {
  return SequenceType::get(element_type);
}

// Primitive types.
#define HANDLE_INTEGER_TYPE(TYPE_NAME, C_TYPE, BITWIDTH, IS_SIGNED)            \
  __RUNTIME_ATTR                                                               \
  type_ref MEMOIR_FUNC(TYPE_NAME##_type)() {                                   \
    return IntegerType::get(BITWIDTH, IS_SIGNED);                              \
  }

#define HANDLE_PRIMITIVE_TYPE(TYPE_NAME, C_TYPE, CLASS_PREFIX)                 \
  __RUNTIME_ATTR                                                               \
  type_ref MEMOIR_FUNC(TYPE_NAME##_type)() {                                   \
    return CLASS_PREFIX##Type::get();                                          \
  }
#include "types.def"

__RUNTIME_ATTR
type_ref MEMOIR_FUNC(void_type)() {
  return VoidType::get();
}

// Derived types.
__RUNTIME_ATTR
type_ref MEMOIR_FUNC(ref_type)(const type_ref referenced_type) {
  return ReferenceType::get(referenced_type);
}

} // extern "C"

} // namespace memoir
