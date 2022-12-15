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

/*
 * Integer accesses
 */
#define HANDLE_INTEGER_TYPE(TYPE_NAME, C_TYPE, BITWIDTH, IS_SIGNED)            \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(struct_read_##TYPE_NAME)(Struct * struct_to_access,       \
                                              unsigned field_index) {          \
    MEMOIR_ACCESS_CHECK(struct_to_access);                                     \
                                                                               \
    auto element = struct_to_access->get_field(field_index);                   \
                                                                               \
    MEMOIR_INTEGER_CHECK(element, BITWIDTH, IS_SIGNED, #TYPE_NAME);            \
                                                                               \
    auto integer_element = static_cast<IntegerElement *>(element);             \
    return (C_TYPE)(integer_element->read_value());                            \
  }                                                                            \
                                                                               \
  __RUNTIME_ATTR                                                               \
  void MEMOIR_FUNC(struct_write_##TYPE_NAME)(C_TYPE value,                     \
                                             Struct * struct_to_access,        \
                                             unsigned field_index) {           \
    MEMOIR_ACCESS_CHECK(struct_to_access);                                     \
                                                                               \
    auto element = struct_to_access->get_field(field_index);                   \
                                                                               \
    MEMOIR_INTEGER_CHECK(element, BITWIDTH, IS_SIGNED, #TYPE_NAME);            \
                                                                               \
    auto integer_element = static_cast<IntegerElement *>(element);             \
    integer_element->write_value((uint64_t)value);                             \
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
    MEMOIR_INTEGER_CHECK(element, BITWIDTH, IS_SIGNED, #TYPE_NAME);            \
                                                                               \
    auto integer_element = static_cast<IntegerElement *>(element);             \
    return (C_TYPE)(integer_element->read_value());                            \
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
    auto element = collection_to_access->get_element(args);                    \
                                                                               \
    va_end(args);                                                              \
                                                                               \
    MEMOIR_INTEGER_CHECK(element, BITWIDTH, IS_SIGNED, #TYPE_NAME);            \
                                                                               \
    auto integer_element = static_cast<IntegerElement *>(element);             \
    integer_element->write_value((uint64_t)value);                             \
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
    MEMOIR_INTEGER_CHECK(element, BITWIDTH, IS_SIGNED, #TYPE_NAME);            \
                                                                               \
    auto integer_element = static_cast<IntegerElement *>(element);             \
    return (C_TYPE)(integer_element->read_value());                            \
  }                                                                            \
                                                                               \
  __RUNTIME_ATTR                                                               \
  void MEMOIR_FUNC(assoc_write_##TYPE_NAME)(C_TYPE value,                      \
                                            Collection * collection_to_access, \
                                            ...) {                             \
    MEMOIR_ACCESS_CHECK(collection_to_access);                                 \
    va_list args;                                                              \
                                                                               \
    va_start(args, collection_to_access);                                      \
                                                                               \
    auto element = collection_to_access->get_element(args);                    \
                                                                               \
    va_end(args);                                                              \
                                                                               \
    MEMOIR_INTEGER_CHECK(element, BITWIDTH, IS_SIGNED, #TYPE_NAME);            \
                                                                               \
    auto integer_element = static_cast<IntegerElement *>(element);             \
    integer_element->write_value((uint64_t)value);                             \
  }

/*
 * Primitive non-integer accesses
 */
#define HANDLE_PRIMITIVE_TYPE(TYPE_NAME, C_TYPE, CLASS)                        \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(struct_read_##TYPE_NAME)(Struct * struct_to_access,       \
                                              unsigned field_index) {          \
    MEMOIR_ACCESS_CHECK(struct_to_access);                                     \
                                                                               \
    auto element = struct_to_access->get_field(field_index);                   \
                                                                               \
    MEMOIR_TYPE_CHECK(element, TypeCode::CLASS##Ty);                           \
                                                                               \
    auto typed_element = static_cast<CLASS##Element *>(element);               \
    return (C_TYPE)typed_element->read_value();                                \
  }                                                                            \
                                                                               \
  __RUNTIME_ATTR                                                               \
  void MEMOIR_FUNC(struct_write_##TYPE_NAME)(C_TYPE value,                     \
                                             Struct * struct_to_access,        \
                                             unsigned field_index) {           \
    MEMOIR_ACCESS_CHECK(struct_to_access);                                     \
                                                                               \
    auto element = struct_to_access->get_field(field_index);                   \
                                                                               \
    MEMOIR_TYPE_CHECK(element, TypeCode::CLASS##Ty);                           \
                                                                               \
    auto typed_element = static_cast<CLASS##Element *>(element);               \
    typed_element->write_value(value);                                         \
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
    MEMOIR_TYPE_CHECK(element, TypeCode::CLASS##Ty);                           \
                                                                               \
    auto typed_element = static_cast<CLASS##Element *>(element);               \
    return (C_TYPE)typed_element->read_value();                                \
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
    auto element = collection_to_access->get_element(args);                    \
                                                                               \
    va_end(args);                                                              \
                                                                               \
    MEMOIR_TYPE_CHECK(element, TypeCode::CLASS##Ty);                           \
                                                                               \
    auto typed_element = static_cast<CLASS##Element *>(element);               \
    typed_element->write_value(value);                                         \
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
    MEMOIR_TYPE_CHECK(element, TypeCode::CLASS##Ty);                           \
                                                                               \
    auto typed_element = static_cast<CLASS##Element *>(element);               \
    return (C_TYPE)typed_element->read_value();                                \
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
    auto element = collection_to_access->get_element(args);                    \
                                                                               \
    va_end(args);                                                              \
                                                                               \
    MEMOIR_TYPE_CHECK(element, TypeCode::CLASS##Ty);                           \
                                                                               \
    auto typed_element = static_cast<CLASS##Element *>(element);               \
    typed_element->write_value(value);                                         \
  }

/*
 * Reference accessors have same handling as primitive types
 */
#define HANDLE_REFERENCE_TYPE(TYPE_NAME, C_TYPE, CLASS_PREFIX)                 \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(struct_read_##TYPE_NAME)(Struct * struct_to_access,       \
                                              unsigned field_index) {          \
    MEMOIR_ACCESS_CHECK(struct_to_access);                                     \
                                                                               \
    auto element = struct_to_access->get_field(field_index);                   \
                                                                               \
    MEMOIR_TYPE_CHECK(element, TypeCode::ReferenceTy);                         \
                                                                               \
    auto typed_element = static_cast<ReferenceElement *>(element);             \
    return (C_TYPE)typed_element->read_value();                                \
  }                                                                            \
                                                                               \
  __RUNTIME_ATTR                                                               \
  void MEMOIR_FUNC(struct_write_##TYPE_NAME)(C_TYPE value,                     \
                                             Struct * struct_to_access,        \
                                             unsigned field_index) {           \
    MEMOIR_ACCESS_CHECK(struct_to_access);                                     \
                                                                               \
    auto element = struct_to_access->get_field(field_index);                   \
                                                                               \
    MEMOIR_TYPE_CHECK(element, TypeCode::ReferenceTy);                         \
                                                                               \
    auto typed_element = static_cast<ReferenceElement *>(element);             \
    typed_element->write_value(value);                                         \
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
    MEMOIR_TYPE_CHECK(element, TypeCode::ReferenceTy);                         \
                                                                               \
    auto typed_element = static_cast<ReferenceElement *>(element);             \
    return (C_TYPE)typed_element->read_value();                                \
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
    auto element = collection_to_access->get_element(args);                    \
                                                                               \
    va_end(args);                                                              \
                                                                               \
    MEMOIR_TYPE_CHECK(element, TypeCode::ReferenceTy);                         \
                                                                               \
    auto typed_element = static_cast<ReferenceElement *>(element);             \
    typed_element->write_value(value);                                         \
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
    MEMOIR_TYPE_CHECK(element, TypeCode::ReferenceTy);                         \
                                                                               \
    auto typed_element = static_cast<ReferenceElement *>(element);             \
    return (C_TYPE)typed_element->read_value();                                \
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
    auto element = collection_to_access->get_element(args);                    \
                                                                               \
    va_end(args);                                                              \
                                                                               \
    MEMOIR_TYPE_CHECK(element, TypeCode::ReferenceTy);                         \
                                                                               \
    auto typed_element = static_cast<ReferenceElement *>(element);             \
    typed_element->write_value(value);                                         \
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
    MEMOIR_##CLASS_PREFIX##_CHECK(element);                                    \
                                                                               \
    auto object_element = static_cast<CLASS_PREFIX##Element *>(element);       \
    return object_element->read_value();                                       \
  }                                                                            \
                                                                               \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(index_get_##TYPE_NAME)(Collection * collection_to_access, \
                                            ...) {                             \
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
    MEMOIR_##CLASS_PREFIX##_CHECK(element);                                    \
                                                                               \
    auto nested_element = static_cast<CLASS_PREFIX##Element *>(element);       \
    return (C_TYPE)(nested_element->read_value());                             \
  }                                                                            \
                                                                               \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(assoc_get_##TYPE_NAME)(Collection * collection_to_access, \
                                            ...) {                             \
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
    MEMOIR_##CLASS_PREFIX##_CHECK(element);                                    \
                                                                               \
    auto nested_element = static_cast<CLASS_PREFIX##Element *>(element);       \
    return (C_TYPE)nested_element->read_value();                               \
  }

#include "types.def"
#undef HANDLE_INTEGER_TYPE
#undef HANDLE_PRIMITIVE_TYPE
#undef HANDLE_REFERENCE_TYPE
#undef HANDLE_NESTED_TYPE

} // extern "C"
} // namespace memoir
