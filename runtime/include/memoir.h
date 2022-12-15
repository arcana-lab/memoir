#ifndef MEMOIR_MEMOIR_H
#define MEMOIR_MEMOIR_H
#pragma once

/*
 * Object representation recognizable by LLVM IR
 * This file describes the API for building and
 * accessing object-ir objects, fields and types
 *
 * Author(s): Tommy McMichen
 * Created: Mar 4, 2022
 */

#include <cstdarg>

#include "objects.h"
#include "types.h"
#include "utils.h"

namespace memoir {
extern "C" {

#define __RUNTIME_ATTR                                                         \
  __declspec(noalias) __attribute__((nothrow)) __attribute__((noinline))       \
      __attribute__((optnone))
#define __ALLOC_ATTR __declspec(allocator)

#define MEMOIR_FUNC(name) memoir__##name

/*
 * Struct Types
 */
__RUNTIME_ATTR
Type *MEMOIR_FUNC(define_struct_type)(const char *name, int num_fields, ...);
#define memoir_define_struct_type(name, ...)                                   \
  MEMOIR_FUNC(define_struct_type)(name, MEMOIR_NARGS(__VA_ARGS__), __VA_ARGS__)

__RUNTIME_ATTR
Type *MEMOIR_FUNC(struct_type)(const char *name);
#define memoir_struct_type(name) MEMOIR_FUNC(struct_type)(name)

/*
 * Static-length Tensor Type
 */
__RUNTIME_ATTR
Type *MEMOIR_FUNC(static_tensor_type)(Type *element_type,
                                      uint64_t num_dimensions,
                                      ...);
#define memoir_static_tensor_type(element_type, ...)                           \
  MEMOIR_FUNC(static_tensor_type)                                              \
  (element_type, MEMOIR_NARGS(__VA_ARGS__), __VA_ARGS__)

/*
 * Collection Types
 */
__RUNTIME_ATTR Type *MEMOIR_FUNC(tensor_type)(Type *element_type,
                                              uint64_t num_dimensions);
#define memoir_tensor_type(element_type, num_dimensions)                       \
  MEMOIR_FUNC(tensor_type)(element_type, num_dimensions)

__RUNTIME_ATTR
Type *MEMOIR_FUNC(assoc_array_type)(Type *key_type, Type *value_type);
#define memoir_assoc_array_type(key_type, value_type)                          \
  MEMOIR_FUNC(assoc_array_type)(key_type, value_type)

__RUNTIME_ATTR
Type *MEMOIR_FUNC(sequence_type)(Type *element_type);
#define memoir_sequence_type(element_type)                                     \
  MEMOIR_FUNC(sequence_type)(element_type)

/*
 * Reference Type
 */
__RUNTIME_ATTR
Type *MEMOIR_FUNC(ref_type)(Type *referenced_type);
#define memoir_ref_t(referenced_type) MEMOIR_FUNC(ref_type)(referenced_type)

/*
 * Primitive Types
 */
#define HANDLE_INTEGER_TYPE(TYPE_NAME, C_TYPE, BITWIDTH, IS_SIGNED)            \
  __RUNTIME_ATTR                                                               \
  Type *MEMOIR_FUNC(TYPE_NAME##_type)();

#define HANDLE_PRIMITIVE_TYPE(TYPE_NAME, C_TYPE, CLASS_PREFIX)                 \
  __RUNTIME_ATTR                                                               \
  Type *MEMOIR_FUNC(TYPE_NAME##_type)();

#define memoir_u64_t MEMOIR_FUNC(u64_type)()
#define memoir_u32_t MEMOIR_FUNC(u32_type)()
#define memoir_u16_t MEMOIR_FUNC(u16_type)()
#define memoir_u8_t MEMOIR_FUNC(u8_type)()
#define memoir_i64_t MEMOIR_FUNC(i64_type)()
#define memoir_i32_t MEMOIR_FUNC(i32_type)()
#define memoir_i16_t MEMOIR_FUNC(i16_type)()
#define memoir_i8_t MEMOIR_FUNC(i8_type)()
#define memoir_bool_t MEMOIR_FUNC(bool_type)()
#define memoir_f32_t MEMOIR_FUNC(f32_type)()
#define memoir_f64_t MEMOIR_FUNC(f64_type)()
#define memoir_ptr_t MEMOIR_FUNC(ptr_type)()

/*
 * Object construction
 */
__ALLOC_ATTR
__RUNTIME_ATTR
Struct *MEMOIR_FUNC(allocate_struct)(Type *type);
#define memoir_allocate_struct(type) MEMOIR_FUNC(allocate_struct)(type)

__ALLOC_ATTR
__RUNTIME_ATTR
Collection *MEMOIR_FUNC(allocate_tensor)(Type *element_type,
                                         uint64_t num_dimensions,
                                         ...);
#define memoir_allocate_tensor(element_type, ...)                              \
  MEMOIR_FUNC(allocate_tensor)                                                 \
  (element_type, MEMOIR_NARGS(__VA_ARGS__), CAST_TO_SIZE_T(__VA_ARGS__))

__ALLOC_ATTR
__RUNTIME_ATTR
Collection *MEMOIR_FUNC(allocate_assoc_array)(Type *key_type, Type *value_type);
#define memoir_allocate_assoc_array(key_type, value_type)                      \
  MEMOIR_FUNC(allocate_assoc_array)(key_type, value_type)

__ALLOC_ATTR
__RUNTIME_ATTR
Collection *MEMOIR_FUNC(allocate_sequence)(Type *element_type,
                                           uint64_t initial_size);
#define memoir_allocate_sequence(element_type, initial_size)                   \
  MEMOIR_FUNC(allocate_sequence)(element_type, initial_size)

/*
 * Object destruction
 */
__RUNTIME_ATTR
void MEMOIR_FUNC(delete_struct)(Struct *object);
#define memoir_delete_struct(strct) MEMOIR_FUNC(delete_struct)(strct)

__RUNTIME_ATTR
void MEMOIR_FUNC(delete_collection)(Collection *collection);
#define memoir_delete_collection(collection)                                   \
  MEMOIR_FUNC(delete_collection)(collection)

/*
 * Collection operations
 */
__RUNTIME_ATTR Collection *MEMOIR_FUNC(
    get_slice)(Collection *collection_to_slice, ...);
#define memoir_sequence_slice(object, left, right)                             \
  MEMOIR_FUNC(get_slice)(object, (int64_t)left, (int64_t)right)

__RUNTIME_ATTR Collection *MEMOIR_FUNC(join)(uint8_t number_of_collections,
                                             Collection *collection_to_join,
                                             ...);
#define memoir_join(object, ...)                                               \
  MEMOIR_FUNC(join)                                                            \
  (1 + MEMOIR_NARGS(__VA_ARGS__), object, __VA_ARGS__)

/*
 * Type checking and function signatures
 */
__RUNTIME_ATTR bool MEMOIR_FUNC(assert_struct_type)(Type *type, Struct *object);
#define memoir_assert_struct_type(type, object)                                \
  MEMOIR_FUNC(assert_struct_type)(type, object)

__RUNTIME_ATTR bool MEMOIR_FUNC(assert_collection_type)(Type *type,
                                                        Collection *object);
#define memoir_assert_collection_type(type, object)                            \
  MEMOIR_FUNC(assert_collection_type)(type, object)

__RUNTIME_ATTR bool MEMOIR_FUNC(set_return_type)(Type *type);
#define memoir_return_type(type) MEMOIR_FUNC(set_return_type)(type)
#define memoir_return(type, object)                                            \
  memoir_return_type(type);                                                    \
  return object

/*
 * Read/Write accesses
 */
#define HANDLE_TYPE(TYPE_NAME, C_TYPE)                                         \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(struct_read_##TYPE_NAME)(Struct * struct_to_access,       \
                                              unsigned field_index);           \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(                                                          \
      index_read_##TYPE_NAME)(Collection * collection_to_access, ...);         \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(                                                          \
      assoc_read_##TYPE_NAME)(Collection * collection_to_access, ...);         \
  __RUNTIME_ATTR                                                               \
  void MEMOIR_FUNC(struct_write_##TYPE_NAME)(C_TYPE value,                     \
                                             Struct * struct_to_access,        \
                                             unsigned field_index);            \
  __RUNTIME_ATTR                                                               \
  void MEMOIR_FUNC(index_write_##TYPE_NAME)(C_TYPE value,                      \
                                            Collection * collection_to_access, \
                                            ...);                              \
  __RUNTIME_ATTR                                                               \
  void MEMOIR_FUNC(assoc_write_##TYPE_NAME)(C_TYPE value,                      \
                                            Collection * collection_to_access, \
                                            ...);

// Nested object access
#define HANDLE_NESTED_TYPE(TYPE_NAME, C_TYPE, CLASS_PREFIX)                    \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(struct_get_##TYPE_NAME)(Struct * struct_to_access,        \
                                             unsigned field_index);            \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(index_get_##TYPE_NAME)(Collection * collection_to_access, \
                                            ...);                              \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(assoc_get_##TYPE_NAME)(Collection * collection_to_access, \
                                            ...);
#include "types.def"

#define memoir_struct_read(ty, strct, field_index)                             \
  MEMOIR_FUNC(struct_read_##ty)(strct, (unsigned)field_index)
#define memoir_index_read(ty, cllct, ...)                                      \
  MEMOIR_FUNC(index_read_##ty)(cllct, CAST_TO_SIZE_T(__VA_ARGS__))
#define memoir_assoc_read(ty, cllct, key)                                      \
  MEMOIR_FUNC(assoc_read_##ty)(cllct, key)
#define memoir_struct_write(ty, val, strct, field_index)                       \
  MEMOIR_FUNC(struct_write_##ty)(val, strct, (unsigned)field_index)
#define memoir_index_write(ty, val, cllct, ...)                                \
  MEMOIR_FUNC(index_write_##ty)(val, cllct, CAST_TO_SIZE_T(__VA_ARGS__))
#define memoir_assoc_write(ty, val, cllct, key)                                \
  MEMOIR_FUNC(assoc_write_##ty)(val, cllct, key)
#define memoir_struct_get(ty, strct, field_index)                              \
  MEMOIR_FUNC(struct_get_##ty)(strct, (unsigned)field_index)
#define memoir_index_get(ty, cllct, ...)                                       \
  MEMOIR_FUNC(index_get_##ty)(cllct, CAST_TO_SIZE_T(__VA_ARGS__))
#define memoir_assoc_get(ty, cllct, key) MEMOIR_FUNC(assoc_get_##ty)(cllct, key)
} // extern "C"
} // namespace memoir

#endif
