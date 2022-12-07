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
#define memoir_ref_type(referenced_type) MEMOIR_FUNC(ref_type)(referenced_type)

/*
 * Primitive Types
 */
__RUNTIME_ATTR
Type *MEMOIR_FUNC(u64_type)();
#define memoir_u64_t MEMOIR_FUNC(u64_type)()
__RUNTIME_ATTR
Type *MEMOIR_FUNC(u32_type)();
#define memoir_u32_t MEMOIR_FUNC(u32_type)()
__RUNTIME_ATTR
Type *MEMOIR_FUNC(u16_type)();
#define memoir_u16_t MEMOIR_FUNC(u16_type)()
__RUNTIME_ATTR
Type *MEMOIR_FUNC(u8_type)();
#define memoir_u8_t MEMOIR_FUNC(u8_type)()
__RUNTIME_ATTR
Type *MEMOIR_FUNC(i64_type)();
#define memoir_i64_t MEMOIR_FUNC(i64_type)()
__RUNTIME_ATTR
Type *MEMOIR_FUNC(i32_type)();
#define memoir_i32_t MEMOIR_FUNC(i32_type)()
__RUNTIME_ATTR
Type *MEMOIR_FUNC(i16_type)();
#define memoir_i16_t MEMOIR_FUNC(i16_type)()
__RUNTIME_ATTR
Type *MEMOIR_FUNC(i8_type)();
#define memoir_i8_t MEMOIR_FUNC(i8_type)()
__RUNTIME_ATTR
Type *MEMOIR_FUNC(bool_type)();
#define memoir_bool_t MEMOIR_FUNC(bool_type)()
__RUNTIME_ATTR
Type *MEMOIR_FUNC(f32_type)();
#define memoir_f32_t MEMOIR_FUNC(f32_type)()
__RUNTIME_ATTR
Type *MEMOIR_FUNC(f64_type)();
#define memoir_f64_t MEMOIR_FUNC(f64_type)()
__RUNTIME_ATTR
Type *MEMOIR_FUNC(ptr_type)();
#define memoir_ptr_t MEMOIR_FUNC(ptr_type)()

/*
 * Object construction
 */
__ALLOC_ATTR
__RUNTIME_ATTR
Object *MEMOIR_FUNC(allocate_struct)(Type *type);
#define memoir_allocate_struct(type) MEMOIR_FUNC(allocate_struct)(type)

__ALLOC_ATTR
__RUNTIME_ATTR
Object *MEMOIR_FUNC(allocate_tensor)(Type *element_type,
                                     uint64_t num_dimensions,
                                     ...);
#define CAST(t, x) (t) x
#define memoir_allocate_tensor(element_type, ...)                              \
  MEMOIR_FUNC(allocate_tensor)                                                 \
  (element_type, MEMOIR_NARGS(__VA_ARGS__), __VA_ARGS__)

__ALLOC_ATTR
__RUNTIME_ATTR
Object *MEMOIR_FUNC(allocate_assoc_array)(Type *key_type, Type *value_type);
#define memoir_allocate_assoc_array(key_type, value_type)                      \
  MEMOIR_FUNC(allocate_assoc_array)(key_type, value_type)

__ALLOC_ATTR
__RUNTIME_ATTR
Object *MEMOIR_FUNC(allocate_sequence)(Type *element_type,
                                       uint64_t initial_size);
#define memoir_allocate_sequence(element_type, initial_size)                   \
  MEMOIR_FUNC(allocate_sequence)(element_type, initial_size)

/*
 * Object destruction
 */
__RUNTIME_ATTR
void MEMOIR_FUNC(delete_object)(Object *object);
#define memoir_delete(object) MEMOIR_FUNC(memoir_delete)(object)

/*
 * Collection operations
 */
__RUNTIME_ATTR
Object *MEMOIR_FUNC(get_slice)(Object *object_to_slice, ...);
#define memoir_sequence_slice(object, left, right)                             \
  MEMOIR_FUNC(get_slice)(object, (int64_t)left, (int64_t)right)

__RUNTIME_ATTR
Object *MEMOIR_FUNC(join)(
    uint8_t number_of_objects, // inclusive of the object_to_join
    Object *object_to_join,
    ...);
#define memoir_join(object, ...)                                               \
  MEMOIR_FUNC(join)                                                            \
  (1 + MEMOIR_NARGS(__VA_ARGS__), object, __VA_ARGS__)

/*
 * Type checking and function signatures
 */
__RUNTIME_ATTR bool MEMOIR_FUNC(assert_type)(Type *type, Object *object);
#define memoir_assert_type(type, object) MEMOIR_FUNC(assert_type)(type, object)

__RUNTIME_ATTR
bool MEMOIR_FUNC(set_return_type)(Type *type);
#define memoir_return_type(type) MEMOIR_FUNC(set_return_type)(type)
#define memoir_return(type, object)                                            \
  memoir_return_type(type);                                                    \
  return object

/*
 * Element accesses
 */
// Unsigned integer access
__RUNTIME_ATTR
void MEMOIR_FUNC(write_u64)(uint64_t value, Object *object_to_access, ...);
#define memoir_write_u64(val, object, ...)                                     \
  MEMOIR_FUNC(write_u64)((uint64_t)val, (Object *)object, __VA_ARGS__)

__RUNTIME_ATTR
void MEMOIR_FUNC(write_u32)(uint32_t value, Object *object_to_access, ...);
#define memoir_write_u64(val, object, ...)                                     \
  MEMOIR_FUNC(write_u64)((uint64_t)val, (Object *)object, __VA_ARGS__)

__RUNTIME_ATTR
void MEMOIR_FUNC(write_u16)(uint16_t value, Object *object_to_access, ...);
#define memoir_write_u64(val, object, ...)                                     \
  MEMOIR_FUNC(write_u64)((uint64_t)val, (Object *)object, __VA_ARGS__)

__RUNTIME_ATTR
void MEMOIR_FUNC(write_u8)(uint8_t value, Object *object_to_access, ...);
#define memoir_write_u64(val, object, ...)                                     \
  MEMOIR_FUNC(write_u64)((uint64_t)val, (Object *)object, __VA_ARGS__)

__RUNTIME_ATTR
uint64_t MEMOIR_FUNC(read_u64)(Object *object_to_access, ...);
#define memoir_read_u64(object, ...)                                           \
  MEMOIR_FUNC(read_u64)((Object *)object, __VA_ARGS__)

__RUNTIME_ATTR
uint32_t MEMOIR_FUNC(read_u32)(Object *object_to_access, ...);
#define memoir_read_u32(object, ...)                                           \
  MEMOIR_FUNC(read_u64)((Object *)object, __VA_ARGS__)

__RUNTIME_ATTR
uint16_t MEMOIR_FUNC(read_u16)(Object *object_to_access, ...);
#define memoir_read_u16(object, ...)                                           \
  MEMOIR_FUNC(read_u16)((Object *)object, __VA_ARGS__)

__RUNTIME_ATTR
uint8_t MEMOIR_FUNC(read_u8)(Object *object_to_access, ...);
#define memoir_read_u8(object, ...)                                            \
  MEMOIR_FUNC(read_u8)((Object *)object, __VA_ARGS__)

// Signed integer access
__RUNTIME_ATTR
void MEMOIR_FUNC(write_i64)(int64_t value, Object *object_to_access, ...);
#define memoir_write_i64(val, object, ...)                                     \
  MEMOIR_FUNC(write_i64)((int64_t)val, (Object *)object, __VA_ARGS__)

__RUNTIME_ATTR
void MEMOIR_FUNC(write_i32)(int32_t value, Object *object_to_access, ...);
#define memoir_write_i32(val, object, ...)                                     \
  MEMOIR_FUNC(write_i32)((int32_t)val, (Object *)object, __VA_ARGS__)

__RUNTIME_ATTR
void MEMOIR_FUNC(write_i16)(int16_t value, Object *object_to_access, ...);
#define memoir_write_i16(val, object, ...)                                     \
  MEMOIR_FUNC(write_i16)((int16_t)val, (Object *)object, __VA_ARGS__)

__RUNTIME_ATTR
void MEMOIR_FUNC(write_i8)(int8_t value, Object *object_to_access, ...);
#define memoir_write_i8(val, object, ...)                                      \
  MEMOIR_FUNC(write_i8)((int8_t)val, (Object *)object, __VA_ARGS__)

__RUNTIME_ATTR
int64_t MEMOIR_FUNC(read_i64)(Object *object_to_access, ...);
#define memoir_read_i64(object, ...)                                           \
  MEMOIR_FUNC(read_i64)((Object *)object, __VA_ARGS__)

__RUNTIME_ATTR
int32_t MEMOIR_FUNC(read_i32)(Object *object_to_access, ...);
#define memoir_read_i32(object, ...)                                           \
  MEMOIR_FUNC(read_i32)((Object *)object, __VA_ARGS__)

__RUNTIME_ATTR
int16_t MEMOIR_FUNC(read_i16)(Object *object_to_access, ...);
#define memoir_read_i16(object, ...)                                           \
  MEMOIR_FUNC(read_i16)((Object *)object, __VA_ARGS__)

__RUNTIME_ATTR
int8_t MEMOIR_FUNC(read_i8)(Object *object_to_access, ...);
#define memoir_read_i8(object, ...)                                            \
  MEMOIR_FUNC(read_i8)((Object *)object, __VA_ARGS__)

// Boolean access
__RUNTIME_ATTR
void MEMOIR_FUNC(write_bool)(bool value, Object *object_to_access, ...);
#define memoir_write_bool(val, object, ...)                                    \
  MEMOIR_FUNC(write_bool)((bool)val, object, __VA_ARGS__)

__RUNTIME_ATTR
bool MEMOIR_FUNC(read_bool)(Object *object_to_access, ...);
#define memoir_read_bool(object, ...)                                          \
  MEMOIR_FUNC(read_bool)                                                       \
  ((Object * object), __VA_ARGS__)

// Floating point access
__RUNTIME_ATTR
void MEMOIR_FUNC(write_f64)(double value, Object *object_to_access, ...);
#define memoir_write_f64(val, object, ...)                                     \
  MEMOIR_FUNC(write_f64)((double)val, (Object *)object, __VA_ARGS__)

__RUNTIME_ATTR
void MEMOIR_FUNC(write_f32)(float value, Object *object_to_access, ...);
#define memoir_write_f32(val, object, ...)                                     \
  MEMOIR_FUNC(write_f32)((float)val, (Object *)object, __VA_ARGS__)

__RUNTIME_ATTR
double MEMOIR_FUNC(read_f64)(Object *object_to_access, ...);
#define memoir_read_f64(object, ...)                                           \
  MEMOIR_FUNC(read_f64)((Object *)object, __VA_ARGS__)

__RUNTIME_ATTR
float MEMOIR_FUNC(read_f32)(Object *object_to_access, ...);
#define memoir_read_f32(object, ...)                                           \
  MEMOIR_FUNC(read_f32)((Object *)object, __VA_ARGS__)

// Reference access
__RUNTIME_ATTR
void MEMOIR_FUNC(write_ref)(Object *object_to_reference,
                            Object *object_to_access,
                            ...);
#define memoir_write_ref(val, obj, ...)                                        \
  MEMOIR_FUNC(write_ref)((Object *)val, (Object *)obj, __VA_ARGS__)

__RUNTIME_ATTR
Object *MEMOIR_FUNC(read_ref)(Object *object_to_access, ...);
#define memoir_read_ref(obj, ...) MEMOIR_FUNC(read_ref)(obj, __VA_ARGS__)

// C Pointer access, these may NOT reference memoir objects
__RUNTIME_ATTR
void MEMOIR_FUNC(write_ptr)(void *pointer, Object *object_to_access, ...);
#define memoir_write_ptr(val, obj, ...)                                        \
  MEMOIR_FUNC(write_ptr)((void *)val, (Object *)obj, __VA_ARGS__)

__RUNTIME_ATTR
void *MEMOIR_FUNC(read_ptr)(Object *object_to_access, ...);
#define memoir_read_ptr(obj, ...)                                              \
  MEMOIR_FUNC(read_ptr)((Object *)obj, __VA_ARGS__)

// Nested object access
__RUNTIME_ATTR
Object *MEMOIR_FUNC(get_object)(Object *object_to_access, ...);
#define memoir_get_object(obj, ...) MEMOIR_FUNC(get_object)(obj, __VA_ARGS__)

} // extern "C"
} // namespace memoir

#endif
