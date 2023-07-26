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

/*
 * Type checking
 */
__RUNTIME_ATTR
bool MEMOIR_FUNC(assert_struct_type)(Type *type, Struct *object) {
  if (object == nullptr) {
    return is_object_type(type);
  }

  MEMOIR_ASSERT((type->equals(object->get_type())),
                "Struct is not the correct type");

  return true;
}

__RUNTIME_ATTR
bool MEMOIR_FUNC(assert_collection_type)(Type *type, Collection *object) {
  if (object == nullptr) {
    return is_object_type(type);
  }

  MEMOIR_ASSERT((type->equals(object->get_type())),
                "Struct is not the correct type");

  return true;
}

__RUNTIME_ATTR
bool MEMOIR_FUNC(set_return_type)(Type *type) {
  return true;
}

__RUNTIME_ATTR
uint64_t MEMOIR_FUNC(size)(Collection *collection) {
  return collection->size();
}

__RUNTIME_ATTR
bool MEMOIR_FUNC(assoc_has)(Collection *collection, ...) {
  MEMOIR_ACCESS_CHECK(collection);

  va_list args;

  va_start(args, collection);

  auto result = collection->has_element(args);

  va_end(args);

  return result;
}

__RUNTIME_ATTR
void MEMOIR_FUNC(assoc_remove)(Collection *collection, ...) {
  MEMOIR_ACCESS_CHECK(collection);

  va_list args;

  va_start(args, collection);

  collection->remove_element(args);

  va_end(args);
}

__RUNTIME_ATTR
Collection *MEMOIR_FUNC(assoc_keys)(Collection *collection) {
  MEMOIR_ACCESS_CHECK(collection);

  MEMOIR_TYPE_CHECK(collection, TypeCode::AssocArrayTy);

  auto *assoc = static_cast<AssocArray *>(collection);

  return assoc->keys();
}

/*
 * Integer accesses
 */
#define HANDLE_INTEGER_TYPE(TYPE_NAME, C_TYPE, BITWIDTH, IS_SIGNED)            \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(struct_read_##TYPE_NAME)(Struct * struct_to_access,       \
                                              unsigned field_index) {          \
    MEMOIR_ACCESS_CHECK(struct_to_access);                                     \
    /* TODO: add field type check. */                                          \
    return (C_TYPE)(struct_to_access->get_field(field_index));                 \
  }                                                                            \
                                                                               \
  __RUNTIME_ATTR                                                               \
  void MEMOIR_FUNC(struct_write_##TYPE_NAME)(C_TYPE value,                     \
                                             Struct * struct_to_access,        \
                                             unsigned field_index) {           \
    MEMOIR_ACCESS_CHECK(struct_to_access);                                     \
    /* TODO: Add field type check. */                                          \
    struct_to_access->set_field((uint64_t)value, field_index);                 \
  }                                                                            \
                                                                               \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(                                                          \
      index_read_##TYPE_NAME)(Collection * collection_to_access, ...) {        \
    MEMOIR_ACCESS_CHECK(collection_to_access);                                 \
                                                                               \
    va_list args;                                                              \
                                                                               \
    va_start(args, collection_to_access);                                      \
                                                                               \
    auto element = collection_to_access->get_element(args);                    \
                                                                               \
    va_end(args);                                                              \
                                                                               \
    return (C_TYPE)element;                                                    \
  }                                                                            \
                                                                               \
  __RUNTIME_ATTR                                                               \
  void MEMOIR_FUNC(index_write_##TYPE_NAME)(C_TYPE value,                      \
                                            Collection * collection_to_access, \
                                            ...) {                             \
    MEMOIR_ACCESS_CHECK(collection_to_access);                                 \
                                                                               \
    va_list args;                                                              \
                                                                               \
    va_start(args, collection_to_access);                                      \
                                                                               \
    collection_to_access->set_element((uint64_t)value, args);                  \
                                                                               \
    va_end(args);                                                              \
  }                                                                            \
                                                                               \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(                                                          \
      assoc_read_##TYPE_NAME)(Collection * collection_to_access, ...) {        \
    MEMOIR_ACCESS_CHECK(collection_to_access);                                 \
                                                                               \
    va_list args;                                                              \
                                                                               \
    va_start(args, collection_to_access);                                      \
                                                                               \
    auto element = collection_to_access->get_element(args);                    \
                                                                               \
    va_end(args);                                                              \
                                                                               \
    return (C_TYPE)element;                                                    \
  }                                                                            \
                                                                               \
  __RUNTIME_ATTR                                                               \
  void MEMOIR_FUNC(assoc_write_##TYPE_NAME)(C_TYPE value,                      \
                                            Collection * collection_to_access, \
                                            ...) {                             \
    MEMOIR_ACCESS_CHECK(collection_to_access);                                 \
    va_list args;                                                              \
    va_start(args, collection_to_access);                                      \
                                                                               \
    collection_to_access->set_element((uint64_t)value, args);                  \
                                                                               \
    va_end(args);                                                              \
  }

/*
 * Primitive non-integer accesses
 */
#define HANDLE_PRIMITIVE_TYPE(TYPE_NAME, C_TYPE, CLASS)                        \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(struct_read_##TYPE_NAME)(Struct * struct_to_access,       \
                                              unsigned field_index) {          \
    MEMOIR_ACCESS_CHECK(struct_to_access);                                     \
    /* TODO: add field type check */                                           \
    return (C_TYPE)struct_to_access->get_field(field_index);                   \
  }                                                                            \
                                                                               \
  __RUNTIME_ATTR                                                               \
  void MEMOIR_FUNC(struct_write_##TYPE_NAME)(C_TYPE value,                     \
                                             Struct * struct_to_access,        \
                                             unsigned field_index) {           \
    MEMOIR_ACCESS_CHECK(struct_to_access);                                     \
    /* TODO: add field type check */                                           \
    struct_to_access->set_field((uint64_t)value, field_index);                 \
  }                                                                            \
                                                                               \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(                                                          \
      index_read_##TYPE_NAME)(Collection * collection_to_access, ...) {        \
    MEMOIR_ACCESS_CHECK(collection_to_access);                                 \
                                                                               \
    va_list args;                                                              \
    va_start(args, collection_to_access);                                      \
    auto element = collection_to_access->get_element(args);                    \
    va_end(args);                                                              \
                                                                               \
    return (C_TYPE)element;                                                    \
  }                                                                            \
                                                                               \
  __RUNTIME_ATTR                                                               \
  void MEMOIR_FUNC(index_write_##TYPE_NAME)(C_TYPE value,                      \
                                            Collection * collection_to_access, \
                                            ...) {                             \
    MEMOIR_ACCESS_CHECK(collection_to_access);                                 \
                                                                               \
    va_list args;                                                              \
    va_start(args, collection_to_access);                                      \
    collection_to_access->set_element((uint64_t)value, args);                  \
    va_end(args);                                                              \
  }                                                                            \
                                                                               \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(                                                          \
      assoc_read_##TYPE_NAME)(Collection * collection_to_access, ...) {        \
    MEMOIR_ACCESS_CHECK(collection_to_access);                                 \
                                                                               \
    va_list args;                                                              \
    va_start(args, collection_to_access);                                      \
    auto element = collection_to_access->get_element(args);                    \
    va_end(args);                                                              \
                                                                               \
    return (C_TYPE)element;                                                    \
  }                                                                            \
                                                                               \
  __RUNTIME_ATTR                                                               \
  void MEMOIR_FUNC(assoc_write_##TYPE_NAME)(C_TYPE value,                      \
                                            Collection * collection_to_access, \
                                            ...) {                             \
    MEMOIR_ACCESS_CHECK(collection_to_access);                                 \
                                                                               \
    va_list args;                                                              \
    va_start(args, collection_to_access);                                      \
    collection_to_access->set_element((uint64_t)value, args);                  \
    va_end(args);                                                              \
  }

/*
 * Reference accessors have same handling as primitive types
 */
#define HANDLE_REFERENCE_TYPE(TYPE_NAME, C_TYPE, CLASS_PREFIX)                 \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(struct_read_##TYPE_NAME)(Struct * struct_to_access,       \
                                              unsigned field_index) {          \
    MEMOIR_ACCESS_CHECK(struct_to_access);                                     \
    /* TODO: add field type check. */                                          \
    return (C_TYPE)struct_to_access->get_field(field_index);                   \
  }                                                                            \
                                                                               \
  __RUNTIME_ATTR                                                               \
  void MEMOIR_FUNC(struct_write_##TYPE_NAME)(C_TYPE value,                     \
                                             Struct * struct_to_access,        \
                                             unsigned field_index) {           \
    MEMOIR_ACCESS_CHECK(struct_to_access);                                     \
    /* TODO: add field type check. */                                          \
    struct_to_access->set_field((uint64_t)value, field_index);                 \
  }                                                                            \
                                                                               \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(                                                          \
      index_read_##TYPE_NAME)(Collection * collection_to_access, ...) {        \
    MEMOIR_ACCESS_CHECK(collection_to_access);                                 \
                                                                               \
    va_list args;                                                              \
    va_start(args, collection_to_access);                                      \
    auto element = collection_to_access->get_element(args);                    \
    va_end(args);                                                              \
                                                                               \
    return (C_TYPE)element;                                                    \
  }                                                                            \
                                                                               \
  __RUNTIME_ATTR                                                               \
  void MEMOIR_FUNC(index_write_##TYPE_NAME)(C_TYPE value,                      \
                                            Collection * collection_to_access, \
                                            ...) {                             \
    MEMOIR_ACCESS_CHECK(collection_to_access);                                 \
                                                                               \
    va_list args;                                                              \
    va_start(args, collection_to_access);                                      \
    collection_to_access->set_element((uint64_t)value, args);                  \
    va_end(args);                                                              \
  }                                                                            \
                                                                               \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(                                                          \
      assoc_read_##TYPE_NAME)(Collection * collection_to_access, ...) {        \
    MEMOIR_ACCESS_CHECK(collection_to_access);                                 \
                                                                               \
    va_list args;                                                              \
                                                                               \
    va_start(args, collection_to_access);                                      \
                                                                               \
    auto element = collection_to_access->get_element(args);                    \
                                                                               \
    va_end(args);                                                              \
    return (C_TYPE)element;                                                    \
  }                                                                            \
                                                                               \
  __RUNTIME_ATTR                                                               \
  void MEMOIR_FUNC(assoc_write_##TYPE_NAME)(C_TYPE value,                      \
                                            Collection * collection_to_access, \
                                            ...) {                             \
    MEMOIR_ACCESS_CHECK(collection_to_access);                                 \
                                                                               \
    va_list args;                                                              \
                                                                               \
    va_start(args, collection_to_access);                                      \
                                                                               \
    collection_to_access->set_element((uint64_t)value, args);                  \
                                                                               \
    va_end(args);                                                              \
  }

/*
 * Nested object accesses
 */
#define MEMOIR_Struct_CHECK MEMOIR_STRUCT_CHECK
#define MEMOIR_Collection_CHECK MEMOIR_COLLECTION_CHECK
#define HANDLE_NESTED_TYPE(TYPE_NAME, C_TYPE, CLASS_PREFIX)                    \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(struct_get_##TYPE_NAME)(Struct * struct_to_access,        \
                                             unsigned field_index) {           \
    MEMOIR_ACCESS_CHECK(struct_to_access);                                     \
                                                                               \
    auto element = struct_to_access->get_field(field_index);                   \
                                                                               \
    return (C_TYPE)element;                                                    \
  }                                                                            \
                                                                               \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(index_get_##TYPE_NAME)(Collection * collection_to_access, \
                                            ...) {                             \
    MEMOIR_ACCESS_CHECK(collection_to_access);                                 \
                                                                               \
    va_list args;                                                              \
    va_start(args, collection_to_access);                                      \
    auto element = collection_to_access->get_element(args);                    \
    va_end(args);                                                              \
                                                                               \
    return (C_TYPE)element;                                                    \
  }                                                                            \
                                                                               \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(assoc_get_##TYPE_NAME)(Collection * collection_to_access, \
                                            ...) {                             \
    MEMOIR_ACCESS_CHECK(collection_to_access);                                 \
                                                                               \
    va_list args;                                                              \
    va_start(args, collection_to_access);                                      \
    auto element = collection_to_access->get_element(args);                    \
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
