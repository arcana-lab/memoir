#ifndef MEMOIR_CMEMOIR_H
#define MEMOIR_CMEMOIR_H
#pragma once

#include "memoir.h"

#include "stddef.h"
#include "stdint.h"

/*
 * C API for the MemOIR compiler instrinsics library.
 *
 * Author(s): Tommy McMichen
 * Created: December 15, 2022
 */

#if defined(__cplusplus)
namespace memoir {
#endif

/*
 * Type definitions
 */
#define memoir_define_struct_type(name, ...)                                   \
  MEMOIR_FUNC(define_struct_type)(name, MEMOIR_NARGS(__VA_ARGS__), __VA_ARGS__)

#define memoir_struct_type(name) MEMOIR_FUNC(struct_type)(name)

/*
 * Derived types
 */
#define memoir_static_tensor_type(element_type, ...)                           \
  MEMOIR_FUNC(static_tensor_type)                                              \
  (element_type, MEMOIR_NARGS(__VA_ARGS__), __VA_ARGS__)

#define memoir_tensor_type(element_type, num_dimensions)                       \
  MEMOIR_FUNC(tensor_type)(element_type, num_dimensions)

#define memoir_assoc_array_type(key_type, value_type)                          \
  MEMOIR_FUNC(assoc_array_type)(key_type, value_type)

#define memoir_sequence_type(element_type)                                     \
  MEMOIR_FUNC(sequence_type)(element_type)

#define memoir_ref_t(referenced_type) MEMOIR_FUNC(ref_type)(referenced_type)

/*
 * Primitive types
 */
#define memoir_u64_t MEMOIR_FUNC(u64_type)()
#define memoir_u32_t MEMOIR_FUNC(u32_type)()
#define memoir_u16_t MEMOIR_FUNC(u16_type)()
#define memoir_u8_t MEMOIR_FUNC(u8_type)()
#define memoir_u2_t MEMOIR_FUNC(u2_type)()
#define memoir_i64_t MEMOIR_FUNC(i64_type)()
#define memoir_i32_t MEMOIR_FUNC(i32_type)()
#define memoir_i16_t MEMOIR_FUNC(i16_type)()
#define memoir_i8_t MEMOIR_FUNC(i8_type)()
#define memoir_i2_t MEMOIR_FUNC(i2_type)()
#define memoir_bool_t MEMOIR_FUNC(boolean_type)()
#define memoir_f32_t MEMOIR_FUNC(f32_type)()
#define memoir_f64_t MEMOIR_FUNC(f64_type)()
#define memoir_ptr_t MEMOIR_FUNC(ptr_type)()

/*
 * Allocation
 */
#define memoir_allocate_struct(type) MEMOIR_FUNC(allocate_struct)(type)

#define memoir_allocate_tensor(element_type, ...)                              \
  MEMOIR_FUNC(allocate_tensor)                                                 \
  (element_type, MEMOIR_NARGS(__VA_ARGS__), MEMOIR_CAST_TO_SIZE_T(__VA_ARGS__))

#define memoir_allocate_sequence(element_type, initial_size)                   \
  MEMOIR_FUNC(allocate_sequence)(element_type, (uint64_t)initial_size)

#define memoir_allocate_assoc_array(key_type, value_type)                      \
  MEMOIR_FUNC(allocate_assoc_array)(key_type, value_type)

#define memoir_delete_struct(strct) MEMOIR_FUNC(delete_struct)(strct)

#define memoir_delete_collection(collection)                                   \
  MEMOIR_FUNC(delete_collection)(collection)

/*
 * Collection operations
 */
#define memoir_size(object) MEMOIR_FUNC(size)(object)

#define memoir_end() MEMOIR_FUNC(end)()

// Immutable sequence operations.
#define memoir_sequence_slice(object, left, right)                             \
  MEMOIR_FUNC(sequence_copy)(object, (size_t)left, (size_t)right)

#define memoir_sequence_copy(object, left, right)                              \
  MEMOIR_FUNC(sequence_copy)(object, (size_t)left, (size_t)right)

#define memoir_seq_copy(object, left, right)                                   \
  MEMOIR_FUNC(sequence_copy)(object, (size_t)left, (size_t)right)

// Mutable sequence operations.
#define memoir_seq_insert(ty, value, object, index)                            \
  MUT_FUNC(sequence_insert_##ty)(value, object, index)

#define memoir_seq_insert_range(object_to_insert, object, index)               \
  MUT_FUNC(sequence_insert)(object_to_insert, object, index)

#define memoir_seq_remove(object, index)                                       \
  MUT_FUNC(sequence_remove)(object, index, index + 1)

#define memoir_seq_remove_range(object, begin, end)                            \
  MUT_FUNC(sequence_remove)(object, begin, end)

#define memoir_seq_append(object, other)                                       \
  MUT_FUNC(sequence_append)(object, other)

#define memoir_seq_swap(object, i, other, other_i)                             \
  MUT_FUNC(sequence_swap)(object, i, i + 1, other, other_i)

#define memoir_seq_swap_range(object, i, j, other, other_i)                    \
  MUT_FUNC(sequence_swap)(object, i, j, other, other_i)

#define memoir_seq_swap_within(object, index, other_index)                     \
  MUT_FUNC(sequence_swap_within)(object, index, index + 1, other_index)

#define memoir_seq_swap_within_range(object, begin, end, other_begin)          \
  MUT_FUNC(sequence_swap_within)(object, begin, end, other_begin)

#define memoir_seq_split(object, i, j) MUT_FUNC(sequence_split)(object, i, j)

// Associative array operations.
#define memoir_assoc_has(object, key) MEMOIR_FUNC(assoc_has)(object, key)

#define memoir_assoc_insert(object, key) MUT_FUNC(assoc_insert)(object, key)

#define memoir_assoc_remove(object, key) MUT_FUNC(assoc_remove)(object, key)

#define memoir_assoc_keys(object) MEMOIR_FUNC(assoc_keys)(object)

/*
 * Type checking
 */
#define memoir_assert_struct_type(type, object)                                \
  MEMOIR_FUNC(assert_struct_type)(type, object)

#define memoir_assert_collection_type(type, object)                            \
  MEMOIR_FUNC(assert_collection_type)(type, object)

#define memoir_return_type(type) MEMOIR_FUNC(set_return_type)(type)
#define memoir_return(type, object)                                            \
  memoir_return_type(type);                                                    \
  return object

/*
 * Read accesses
 */
#define memoir_struct_read(ty, strct, field_index)                             \
  MEMOIR_FUNC(struct_read_##ty)(strct, (unsigned)field_index)
#define memoir_index_read(ty, cllct, ...)                                      \
  MEMOIR_FUNC(index_read_##ty)(cllct, MEMOIR_CAST_TO_SIZE_T(__VA_ARGS__))
#define memoir_assoc_read(ty, cllct, key)                                      \
  MEMOIR_FUNC(assoc_read_##ty)(cllct, key)

/*
 * Write accesses
 */
#define memoir_struct_write(ty, val, strct, field_index)                       \
  MUT_FUNC(struct_write_##ty)(val, strct, (unsigned)field_index)
#define memoir_index_write(ty, val, cllct, ...)                                \
  MUT_FUNC(index_write_##ty)(val, cllct, MEMOIR_CAST_TO_SIZE_T(__VA_ARGS__))
#define memoir_assoc_write(ty, val, cllct, key)                                \
  MUT_FUNC(assoc_write_##ty)(val, cllct, key)

/*
 * Nested struct/collection accesses
 */
#define memoir_struct_get(ty, strct, field_index)                              \
  MEMOIR_FUNC(struct_get_##ty)(strct, (unsigned)field_index)
#define memoir_index_get(ty, cllct, ...)                                       \
  MEMOIR_FUNC(index_get_##ty)(cllct, MEMOIR_CAST_TO_SIZE_T(__VA_ARGS__))
#define memoir_assoc_get(ty, cllct, key) MEMOIR_FUNC(assoc_get_##ty)(cllct, key)

#if defined(__cplusplus)
} // namespace memoir
#endif

#endif
