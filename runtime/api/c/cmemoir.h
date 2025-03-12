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
#if 0
#  define memoir_define_type(name, type) MEMOIR_FUNC(define_type)(name, type)

#  define memoir_lookup_type(name) MEMOIR_FUNC(lookup_type)(name)
#endif

/*
 * Derived types
 */
#define memoir_tuple_type(types...) MEMOIR_FUNC(tuple_type)(types)

#define memoir_static_tensor_type(element_type, length)                        \
  MEMOIR_FUNC(array_type)(element_type, (size_t)length)

#define memoir_assoc_type(key_type, value_type)                                \
  MEMOIR_FUNC(assoc_type)(key_type, value_type)

#define memoir_assoc_array_type(key_type, value_type)                          \
  memoir_assoc_type(key_type, value_type)

#define memoir_set_type(key_type) memoir_assoc_type(key_type, memoir_void_t)

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
#define memoir_void_t MEMOIR_FUNC(void_type)()

/*
 * Allocation
 */
#define memoir_allocate(type, args...) MEMOIR_FUNC(allocate)(type, ##args)

#define memoir_allocate_struct(type) memoir_allocate(type)

#define memoir_allocate_sequence(element_type, initial_size)                   \
  memoir_allocate(memoir_sequence_type(element_type), (size_t)initial_size)

#define memoir_allocate_assoc(key_type, value_type)                            \
  memoir_allocate(memoir_assoc_type(key_type, value_type))

#define memoir_allocate_assoc_array(key_type, value_type)                      \
  memoir_allocate_assoc(key_type, value_type)

#define memoir_allocate_set(key_type)                                          \
  memoir_allocate_assoc(key_type, memoir_void_t)

#define memoir_selection(_sel) MEMOIR_KEYWORD(selection), _sel

#define memoir_delete(collection) MEMOIR_FUNC(delete)(collection)

#define memoir_delete_struct(strct) memoir_delete(strct)

#define memoir_delete_collection(collection) memoir_delete(collection)

/*
 * Collection operations
 */
#define memoir_size(_obj, _args...) MEMOIR_FUNC(size)(_obj, ##_args)

#define memoir_clear(_obj, _args...) MUT_FUNC(clear)(_obj, ##_args)

#define memoir_end() MEMOIR_FUNC(end)()

#define memoir_closed(_closed...) MEMOIR_KEYWORD(closed), ##_closed

#define memoir_fold(_ty, _f, _accum, _collection, _args...)                    \
  MEMOIR_FUNC(fold_##_ty)                                                      \
  ((void *)_f, _accum, _collection, ##_args)

#define memoir_rfold(_ty, _f, _accum, _collection, _args...)                   \
  MEMOIR_FUNC(rfold_##_ty)                                                     \
  ((void *)_f, _accum, _collection, ##_args)

// Immutable sequence operations.
#define memoir_copy(object, _args...) MEMOIR_FUNC(copy)(object, ##_args)

#define memoir_sequence_slice(object, left, right)                             \
  MEMOIR_FUNC(copy)(object, MEMOIR_KEYWORD(range), (size_t)left, (size_t)right)

#define memoir_sequence_copy(object, left, right)                              \
  MEMOIR_FUNC(copy)(object, MEMOIR_KEYWORD(range), (size_t)left, (size_t)right)

#define memoir_seq_copy(object, left, right)                                   \
  MEMOIR_FUNC(copy)(object, MEMOIR_KEYWORD(range), (size_t)left, (size_t)right)

// Insert operations.
#define memoir_insert(_object, _args...) MUT_FUNC(insert)(_object, ##_args)

#define memoir_insert_value(_value, _object, _indices...)                      \
  memoir_insert(_object, ##_indices, MEMOIR_KEYWORD(value), _value)

#define memoir_push_back(_value, _object, _indices...)                         \
  memoir_insert(_object,                                                       \
                memoir_end(),                                                  \
                ##_indices,                                                    \
                MEMOIR_KEYWORD(value),                                         \
                _value)

#define memoir_range(_from, _to)                                               \
  MEMOIR_KEYWORD(range), (size_t)_from, (size_t)_to

#define memoir_input(_obj, _args...) MEMOIR_KEYWORD(input), _obj, ##_args

#define memoir_value(_val) MEMOIR_KEYWORD(value), _val

#define memoir_input_range(_obj, _from, _to)                                   \
  memoir_input(_obj), memoir_range(_from, _to)

#define memoir_seq_insert_range(_to_insert, _obj, _indices...)                 \
  memoir_insert(_obj, _indices, memoir_input(_to_insert))

#define memoir_seq_append(object, other)                                       \
  memoir_insert(object, memoir_end(), memoir_input(other))

// Removal operations.
#define memoir_remove(_obj, _args...) MUT_FUNC(remove)(_obj, ##_args)

#define memoir_seq_remove(_obj, _indices...) memoir_remove(_obj, _indices)

#define memoir_seq_remove_range(_obj, _begin, _end, _indices)                  \
  memoir_remove(_obj, ##_indices, memoir_range(_begin, _end))

#define memoir_seq_pop_back(_obj, _indices...)                                 \
  memoir_remove(_obj, ##_indices, memoir_end())

// Associative array operations.
#define memoir_has(_obj, _args...) MEMOIR_FUNC(has)(_obj, ##_args)

#define memoir_assoc_has(_obj, _args...) MEMOIR_FUNC(has)(_obj, ##_args)

#define memoir_assoc_insert(_obj, _args...) memoir_insert(_obj, ##_args)

#define memoir_assoc_remove(_obj, _args...) memoir_remove(_obj, ##_args)

#define memoir_assoc_keys(_obj, _args...) MEMOIR_FUNC(assoc_keys)(_obj, ##_args)

/*
 * Type checking
 */
#define memoir_assert_type(_type, _obj) MEMOIR_FUNC(assert_type)(_type, _obj)

#define memoir_assert_struct_type(type, object) memoir_assert_type(type, object)

#define memoir_assert_collection_type(type, object)                            \
  memoir_assert_type(type, object)

#define memoir_return_type(type) MEMOIR_FUNC(return_type)(type)
#define memoir_return(type, object)                                            \
  memoir_return_type(type);                                                    \
  return object

/*
 * Read accesses
 */
#define memoir_read(_ty, _cllct, _args...)                                     \
  MEMOIR_FUNC(read_##_ty)(_cllct, ##_args)

#define memoir_struct_read(ty, strct, field_index, ...)                        \
  memoir_read(ty, strct, (unsigned)field_index, ##__VA_ARGS__)
#define memoir_index_read(ty, cllct, index, ...)                               \
  memoir_read(ty, cllct, (size_t)index, ##__VA_ARGS__)
#define memoir_assoc_read(ty, cllct, key, ...)                                 \
  memoir_read(ty, cllct, key, ##__VA_ARGS__)

/*
 * Write accesses
 */
#define memoir_write(_ty, _cllct, _args...)                                    \
  MUT_FUNC(write_##_ty)(_cllct, ##_args)

#define memoir_struct_write(ty, val, strct, field_index, ...)                  \
  memoir_write(ty, val, strct, (unsigned)field_index, ##__VA_ARGS__)
#define memoir_index_write(ty, val, cllct, index, ...)                         \
  memoir_write(ty, val, cllct, (size_t)index, ##__VA_ARGS__)
#define memoir_assoc_write(ty, val, cllct, key, ...)                           \
  memoir_write(ty, val, cllct, key, ##__VA_ARGS__)

/*
 * Nested struct/collection accesses
 */
#define memoir_get(_c, _args...) MEMOIR_FUNC(get)(_c, ##_args)

#define memoir_struct_get(ty, strct, field_index)                              \
  memoir_get(strct, (unsigned)field_index)
#define memoir_index_get(ty, cllct, ...)                                       \
  memoir_get(cllct, MEMOIR_CAST_TO_SIZE_T(__VA_ARGS__))
#define memoir_assoc_get(ty, cllct, key) memoir_get(cllct, key)

#if defined(__cplusplus)
} // namespace memoir
#endif

#endif
