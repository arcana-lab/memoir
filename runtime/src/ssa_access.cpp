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

// Collection operations.

// Integer accesses
#define HANDLE_INTEGER_TYPE(TYPE_NAME, C_TYPE, BITWIDTH, IS_SIGNED)            \
                                                                               \
  __IMMUT_ATTR                                                                 \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(struct_read_##TYPE_NAME)(                                 \
      const struct_ref struct_to_access,                                       \
      unsigned field_index) {                                                  \
    MEMOIR_ACCESS_CHECK(struct_to_access);                                     \
    /* TODO: add field type check. */                                          \
    return (C_TYPE)(                                                           \
        ((detail::Struct *)struct_to_access)->get_field(field_index));         \
  }                                                                            \
                                                                               \
  __RUNTIME_ATTR                                                               \
  void MEMOIR_FUNC(struct_write_##TYPE_NAME)(C_TYPE value,                     \
                                             struct_ref struct_to_access,      \
                                             unsigned field_index) {           \
    MEMOIR_ACCESS_CHECK(struct_to_access);                                     \
    /* TODO: Add field type check. */                                          \
    ((detail::Struct *)struct_to_access)                                       \
        ->set_field((uint64_t)value, field_index);                             \
  }                                                                            \
                                                                               \
  __IMMUT_ATTR                                                                 \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(index_read_##TYPE_NAME)(                                  \
      const collection_ref collection_to_access,                               \
      ...) {                                                                   \
    MEMOIR_ACCESS_CHECK(collection_to_access);                                 \
                                                                               \
    va_list args;                                                              \
                                                                               \
    va_start(args, collection_to_access);                                      \
                                                                               \
    auto element =                                                             \
        ((detail::Collection *)collection_to_access)->get_element(args);       \
                                                                               \
    va_end(args);                                                              \
                                                                               \
    return (C_TYPE)element;                                                    \
  }                                                                            \
                                                                               \
  __IMMUT_ATTR                                                                 \
  __ALLOC_ATTR                                                                 \
  __RUNTIME_ATTR                                                               \
  collection_ref MEMOIR_FUNC(index_write_##TYPE_NAME)(                         \
      C_TYPE value,                                                            \
      const collection_ref collection_to_access,                               \
      ...) {                                                                   \
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
    return collection_to_access;                                               \
  }                                                                            \
                                                                               \
  __IMMUT_ATTR                                                                 \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(                                                          \
      assoc_read_##TYPE_NAME)(collection_ref collection_to_access, ...) {      \
    MEMOIR_ACCESS_CHECK(collection_to_access);                                 \
                                                                               \
    va_list args;                                                              \
                                                                               \
    va_start(args, collection_to_access);                                      \
                                                                               \
    auto element =                                                             \
        ((detail::Collection *)collection_to_access)->get_element(args);       \
                                                                               \
    va_end(args);                                                              \
                                                                               \
    return (C_TYPE)element;                                                    \
  }                                                                            \
                                                                               \
  __IMMUT_ATTR                                                                 \
  __ALLOC_ATTR                                                                 \
  __RUNTIME_ATTR                                                               \
  collection_ref MEMOIR_FUNC(assoc_write_##TYPE_NAME)(                         \
      C_TYPE value,                                                            \
      const collection_ref collection_to_access,                               \
      ...) {                                                                   \
    MEMOIR_ACCESS_CHECK(collection_to_access);                                 \
    va_list args;                                                              \
    va_start(args, collection_to_access);                                      \
                                                                               \
    ((detail::Collection *)collection_to_access)                               \
        ->set_element((uint64_t)value, args);                                  \
                                                                               \
    va_end(args);                                                              \
    return collection_to_access;                                               \
  }

// Primitive non-integer accesses
#define HANDLE_PRIMITIVE_TYPE(TYPE_NAME, C_TYPE, CLASS)                        \
                                                                               \
  __IMMUT_ATTR                                                                 \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(struct_read_##TYPE_NAME)(struct_ref struct_to_access,     \
                                              unsigned field_index) {          \
    MEMOIR_ACCESS_CHECK(struct_to_access);                                     \
    /* TODO: add field type check */                                           \
    return (C_TYPE)((detail::Struct *)struct_to_access)                        \
        ->get_field(field_index);                                              \
  }                                                                            \
                                                                               \
  __RUNTIME_ATTR                                                               \
  void MEMOIR_FUNC(struct_write_##TYPE_NAME)(C_TYPE value,                     \
                                             struct_ref struct_to_access,      \
                                             unsigned field_index) {           \
    MEMOIR_ACCESS_CHECK(struct_to_access);                                     \
    /* TODO: add field type check */                                           \
    ((detail::Struct *)struct_to_access)                                       \
        ->set_field((uint64_t)value, field_index);                             \
  }                                                                            \
                                                                               \
  __IMMUT_ATTR                                                                 \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(                                                          \
      index_read_##TYPE_NAME)(collection_ref collection_to_access, ...) {      \
    MEMOIR_ACCESS_CHECK(collection_to_access);                                 \
                                                                               \
    va_list args;                                                              \
    va_start(args, collection_to_access);                                      \
    auto element =                                                             \
        ((detail::Collection *)collection_to_access)->get_element(args);       \
    va_end(args);                                                              \
                                                                               \
    return (C_TYPE)element;                                                    \
  }                                                                            \
                                                                               \
  __IMMUT_ATTR                                                                 \
  __ALLOC_ATTR                                                                 \
  __RUNTIME_ATTR                                                               \
  collection_ref MEMOIR_FUNC(index_write_##TYPE_NAME)(                         \
      C_TYPE value,                                                            \
      const collection_ref collection_to_access,                               \
      ...) {                                                                   \
    MEMOIR_ACCESS_CHECK(collection_to_access);                                 \
                                                                               \
    va_list args;                                                              \
    va_start(args, collection_to_access);                                      \
    ((detail::Collection *)collection_to_access)                               \
        ->set_element((uint64_t)value, args);                                  \
    va_end(args);                                                              \
    return collection_to_access;                                               \
  }                                                                            \
                                                                               \
  __IMMUT_ATTR                                                                 \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(                                                          \
      assoc_read_##TYPE_NAME)(collection_ref collection_to_access, ...) {      \
    MEMOIR_ACCESS_CHECK(collection_to_access);                                 \
                                                                               \
    va_list args;                                                              \
    va_start(args, collection_to_access);                                      \
    auto element =                                                             \
        ((detail::Collection *)collection_to_access)->get_element(args);       \
    va_end(args);                                                              \
                                                                               \
    return (C_TYPE)element;                                                    \
  }                                                                            \
                                                                               \
  __IMMUT_ATTR                                                                 \
  __ALLOC_ATTR                                                                 \
  __RUNTIME_ATTR                                                               \
  collection_ref MEMOIR_FUNC(assoc_write_##TYPE_NAME)(                         \
      C_TYPE value,                                                            \
      const collection_ref collection_to_access,                               \
      ...) {                                                                   \
    MEMOIR_ACCESS_CHECK(collection_to_access);                                 \
                                                                               \
    va_list args;                                                              \
    va_start(args, collection_to_access);                                      \
    ((detail::Collection *)collection_to_access)                               \
        ->set_element((uint64_t)value, args);                                  \
    va_end(args);                                                              \
    return collection_to_access;                                               \
  }

// Reference accessors have same handling as primitive types
#define HANDLE_REFERENCE_TYPE(TYPE_NAME, C_TYPE, CLASS_PREFIX)                 \
  __IMMUT_ATTR                                                                 \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(struct_read_##TYPE_NAME)(                                 \
      const struct_ref struct_to_access,                                       \
      unsigned field_index) {                                                  \
    MEMOIR_ACCESS_CHECK(struct_to_access);                                     \
    /* TODO: add field type check. */                                          \
    return (C_TYPE)((detail::Struct *)struct_to_access)                        \
        ->get_field(field_index);                                              \
  }                                                                            \
                                                                               \
  __RUNTIME_ATTR                                                               \
  void MEMOIR_FUNC(struct_write_##TYPE_NAME)(C_TYPE value,                     \
                                             struct_ref struct_to_access,      \
                                             unsigned field_index) {           \
    MEMOIR_ACCESS_CHECK(struct_to_access);                                     \
    /* TODO: add field type check. */                                          \
    ((detail::Struct *)struct_to_access)                                       \
        ->set_field((uint64_t)value, field_index);                             \
  }                                                                            \
                                                                               \
  __IMMUT_ATTR                                                                 \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(index_read_##TYPE_NAME)(                                  \
      const collection_ref collection_to_access,                               \
      ...) {                                                                   \
    MEMOIR_ACCESS_CHECK(collection_to_access);                                 \
                                                                               \
    va_list args;                                                              \
    va_start(args, collection_to_access);                                      \
    auto element =                                                             \
        ((detail::Collection *)collection_to_access)->get_element(args);       \
    va_end(args);                                                              \
                                                                               \
    return (C_TYPE)element;                                                    \
  }                                                                            \
                                                                               \
  __IMMUT_ATTR                                                                 \
  __ALLOC_ATTR                                                                 \
  __RUNTIME_ATTR                                                               \
  collection_ref MEMOIR_FUNC(index_write_##TYPE_NAME)(                         \
      C_TYPE value,                                                            \
      const collection_ref collection_to_access,                               \
      ...) {                                                                   \
    MEMOIR_ACCESS_CHECK(collection_to_access);                                 \
                                                                               \
    va_list args;                                                              \
    va_start(args, collection_to_access);                                      \
    ((detail::Collection *)collection_to_access)                               \
        ->set_element((uint64_t)value, args);                                  \
    va_end(args);                                                              \
    return collection_to_access;                                               \
  }                                                                            \
                                                                               \
  __IMMUT_ATTR                                                                 \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(assoc_read_##TYPE_NAME)(                                  \
      const collection_ref collection_to_access,                               \
      ...) {                                                                   \
    MEMOIR_ACCESS_CHECK(collection_to_access);                                 \
                                                                               \
    va_list args;                                                              \
                                                                               \
    va_start(args, collection_to_access);                                      \
                                                                               \
    auto element =                                                             \
        ((detail::Collection *)collection_to_access)->get_element(args);       \
                                                                               \
    va_end(args);                                                              \
    return (C_TYPE)element;                                                    \
  }                                                                            \
                                                                               \
  __IMMUT_ATTR                                                                 \
  __ALLOC_ATTR                                                                 \
  __RUNTIME_ATTR                                                               \
  collection_ref MEMOIR_FUNC(assoc_write_##TYPE_NAME)(                         \
      C_TYPE value,                                                            \
      const collection_ref collection_to_access,                               \
      ...) {                                                                   \
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
    return collection_to_access;                                               \
  }

// Nested object accesses
#define MEMOIR_Struct_CHECK MEMOIR_STRUCT_CHECK
#define MEMOIR_Collection_CHECK MEMOIR_COLLECTION_CHECK
#define HANDLE_NESTED_TYPE(TYPE_NAME, C_TYPE, CLASS_PREFIX)                    \
  __IMMUT_ATTR                                                                 \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(struct_get_##TYPE_NAME)(                                  \
      const struct_ref struct_to_access,                                       \
      unsigned field_index) {                                                  \
    MEMOIR_ACCESS_CHECK(struct_to_access);                                     \
                                                                               \
    auto element =                                                             \
        ((detail::Struct *)struct_to_access)->get_field(field_index);          \
                                                                               \
    return (C_TYPE)element;                                                    \
  }                                                                            \
                                                                               \
  __IMMUT_ATTR                                                                 \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(                                                          \
      index_get_##TYPE_NAME)(const collection_ref collection_to_access, ...) { \
    MEMOIR_ACCESS_CHECK(collection_to_access);                                 \
                                                                               \
    va_list args;                                                              \
    va_start(args, collection_to_access);                                      \
    auto element =                                                             \
        ((detail::Collection *)collection_to_access)->get_element(args);       \
    va_end(args);                                                              \
                                                                               \
    return (C_TYPE)element;                                                    \
  }                                                                            \
                                                                               \
  __IMMUT_ATTR                                                                 \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(                                                          \
      assoc_get_##TYPE_NAME)(const collection_ref collection_to_access, ...) { \
    MEMOIR_ACCESS_CHECK(collection_to_access);                                 \
                                                                               \
    va_list args;                                                              \
    va_start(args, collection_to_access);                                      \
    auto element =                                                             \
        ((detail::Collection *)collection_to_access)->get_element(args);       \
    va_end(args);                                                              \
                                                                               \
    return (C_TYPE)element;                                                    \
  }

#include "types.def"
#undef HANDLE_INTEGER_TYPE
#undef HANDLE_PRIMITIVE_TYPE
#undef HANDLE_REFERENCE_TYPE
#undef HANDLE_NESTED_TYPE

} // extern "C"
} // namespace memoir
