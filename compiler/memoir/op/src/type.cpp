#include "internal.h"
#include "memoir.h"
#include "types.h"

namespace memoir {

extern "C" {

// User-defined types.
#if 0
__RUNTIME_ATTR
type_ref MEMOIR_FUNC(define_type)(const char *name, const type_ref type) {
  return nullptr;
}

type_ref MEMOIR_FUNC(lookup_type)(const char *name) {
  return nullptr;
}
#endif

__RUNTIME_ATTR
type_ref MEMOIR_FUNC(tuple_type)(const type_ref first, ...) {
  return nullptr;
}

// Collection types.
__RUNTIME_ATTR
type_ref MEMOIR_FUNC(array_type)(type_ref type, size_t length) {
  return SequenceType::get(type);
}

__RUNTIME_ATTR
type_ref MEMOIR_FUNC(assoc_type)(const type_ref key_type,
                                 const type_ref value_type,
                                 ...) {
  return AssocArrayType::get(key_type, value_type);
}

__RUNTIME_ATTR
type_ref MEMOIR_FUNC(sequence_type)(const type_ref element_type, ...) {
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
