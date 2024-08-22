/*
 * Object representation recognizable by LLVM IR
 * This file contains methods to access object-ir
 * objects, fields and types for the object-ir library.
 *
 * Author(s): Tommy McMichen
 * Created: Mar 17, 2022
 */

#include <cassert>
#include <iostream>

#include "internal.h"
#include "memoir.h"

namespace memoir {

extern "C" {

// Integer accesses
#define HANDLE_INTEGER_TYPE(TYPE_NAME, C_TYPE, BITWIDTH, IS_SIGNED)            \
  __RUNTIME_ATTR                                                               \
  void MUT_FUNC(struct_write_##TYPE_NAME)(C_TYPE value,                        \
                                          struct_ref struct_to_access,         \
                                          unsigned field_index,                \
                                          ...) {                               \
    MEMOIR_ACCESS_CHECK(struct_to_access);                                     \
    /* TODO: Add field type check. */                                          \
    ((detail::Struct *)struct_to_access)                                       \
        ->set_field((uint64_t)value, field_index);                             \
  }                                                                            \
                                                                               \
  __RUNTIME_ATTR                                                               \
  void MUT_FUNC(index_write_##TYPE_NAME)(C_TYPE value,                         \
                                         collection_ref collection_to_access,  \
                                         ...) {                                \
    MEMOIR_ACCESS_CHECK(collection_to_access);                                 \
                                                                               \
    va_list args;                                                              \
                                                                               \
    va_start(args, collection_to_access);                                      \
                                                                               \
    ((detail::Collection *)collection_to_access)                               \
        ->set_element((uint64_t)value, args);                                  \
                                                                               \
    va_end(args);                                                              \
  }                                                                            \
                                                                               \
  __RUNTIME_ATTR                                                               \
  void MUT_FUNC(assoc_write_##TYPE_NAME)(C_TYPE value,                         \
                                         collection_ref collection_to_access,  \
                                         ...) {                                \
    MEMOIR_ACCESS_CHECK(collection_to_access);                                 \
    va_list args;                                                              \
    va_start(args, collection_to_access);                                      \
                                                                               \
    ((detail::Collection *)collection_to_access)                               \
        ->set_element((uint64_t)value, args);                                  \
                                                                               \
    va_end(args);                                                              \
  }

// Primitive non-integer accesses
#define HANDLE_PRIMITIVE_TYPE(TYPE_NAME, C_TYPE, CLASS)                        \
  __RUNTIME_ATTR                                                               \
  void MUT_FUNC(struct_write_##TYPE_NAME)(C_TYPE value,                        \
                                          struct_ref struct_to_access,         \
                                          unsigned field_index,                \
                                          ...) {                               \
    MEMOIR_ACCESS_CHECK(struct_to_access);                                     \
    /* TODO: add field type check */                                           \
    ((detail::Struct *)struct_to_access)                                       \
        ->set_field((uint64_t)value, field_index);                             \
  }                                                                            \
                                                                               \
  __RUNTIME_ATTR                                                               \
  void MUT_FUNC(index_write_##TYPE_NAME)(C_TYPE value,                         \
                                         collection_ref collection_to_access,  \
                                         ...) {                                \
    MEMOIR_ACCESS_CHECK(collection_to_access);                                 \
                                                                               \
    va_list args;                                                              \
    va_start(args, collection_to_access);                                      \
    ((detail::Collection *)collection_to_access)                               \
        ->set_element((uint64_t)value, args);                                  \
    va_end(args);                                                              \
  }                                                                            \
                                                                               \
  __RUNTIME_ATTR                                                               \
  void MUT_FUNC(assoc_write_##TYPE_NAME)(C_TYPE value,                         \
                                         collection_ref collection_to_access,  \
                                         ...) {                                \
    MEMOIR_ACCESS_CHECK(collection_to_access);                                 \
                                                                               \
    va_list args;                                                              \
    va_start(args, collection_to_access);                                      \
    ((detail::Collection *)collection_to_access)                               \
        ->set_element((uint64_t)value, args);                                  \
    va_end(args);                                                              \
  }

// Reference accessors have same handling as primitive types
#define HANDLE_REFERENCE_TYPE(TYPE_NAME, C_TYPE, CLASS_PREFIX)                 \
  __RUNTIME_ATTR                                                               \
  void MUT_FUNC(struct_write_##TYPE_NAME)(C_TYPE value,                        \
                                          struct_ref struct_to_access,         \
                                          unsigned field_index,                \
                                          ...) {                               \
    MEMOIR_ACCESS_CHECK(struct_to_access);                                     \
    /* TODO: add field type check. */                                          \
    ((detail::Struct *)struct_to_access)                                       \
        ->set_field((uint64_t)value, field_index);                             \
  }                                                                            \
                                                                               \
  __RUNTIME_ATTR                                                               \
  void MUT_FUNC(index_write_##TYPE_NAME)(C_TYPE value,                         \
                                         collection_ref collection_to_access,  \
                                         ...) {                                \
    MEMOIR_ACCESS_CHECK(collection_to_access);                                 \
                                                                               \
    va_list args;                                                              \
    va_start(args, collection_to_access);                                      \
    ((detail::Collection *)collection_to_access)                               \
        ->set_element((uint64_t)value, args);                                  \
    va_end(args);                                                              \
  }                                                                            \
                                                                               \
  __RUNTIME_ATTR                                                               \
  void MUT_FUNC(assoc_write_##TYPE_NAME)(C_TYPE value,                         \
                                         collection_ref collection_to_access,  \
                                         ...) {                                \
    MEMOIR_ACCESS_CHECK(collection_to_access);                                 \
                                                                               \
    va_list args;                                                              \
                                                                               \
    va_start(args, collection_to_access);                                      \
                                                                               \
    ((detail::Collection *)collection_to_access)                               \
        ->set_element((uint64_t)value, args);                                  \
                                                                               \
    va_end(args);                                                              \
  }

#include "types.def"
#undef HANDLE_INTEGER_TYPE
#undef HANDLE_PRIMITIVE_TYPE
#undef HANDLE_REFERENCE_TYPE

} // extern "C"
} // namespace memoir
