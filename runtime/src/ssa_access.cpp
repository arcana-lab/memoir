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

#include "objects.h"

namespace memoir {

extern "C" {

// Collection operations.

// Integer accesses
#define HANDLE_INTEGER_TYPE(TYPE_NAME, C_TYPE, BITWIDTH, IS_SIGNED)            \
                                                                               \
  __IMMUT_ATTR                                                                 \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(read_##TYPE_NAME)(collection_ref collection_to_access,    \
                                       ...) {                                  \
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
  collection_ref MEMOIR_FUNC(write_##TYPE_NAME)(                               \
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
  C_TYPE MEMOIR_FUNC(read_##TYPE_NAME)(collection_ref collection_to_access,    \
                                       ...) {                                  \
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
  collection_ref MEMOIR_FUNC(write_##TYPE_NAME)(                               \
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
  C_TYPE MEMOIR_FUNC(                                                          \
      read_##TYPE_NAME)(const collection_ref collection_to_access, ...) {      \
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
  collection_ref MEMOIR_FUNC(write_##TYPE_NAME)(                               \
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

#include "types.def"
#undef HANDLE_INTEGER_TYPE
#undef HANDLE_PRIMITIVE_TYPE
#undef HANDLE_REFERENCE_TYPE
#undef HANDLE_NESTED_TYPE

// Nested object accesses
__IMMUT_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(get)(const collection_ref collection_to_access,
                                ...) {
  MEMOIR_ACCESS_CHECK(collection_to_access);

  va_list args;
  va_start(args, collection_to_access);
  auto element =
      ((detail::Collection *)collection_to_access)->get_element(args);
  va_end(args);

  return (collection_ref)element;
}

} // extern "C"
} // namespace memoir
