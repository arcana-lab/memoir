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

namespace memoir {
extern "C" {

#define __RUNTIME_ATTR                                                         \
  __declspec(noalias) __attribute__((nothrow)) __attribute__((noinline))       \
      __attribute__((optnone))
#define __ALLOC_ATTR __declspec(allocator)

#define MEMOIR_FUNC(name) memoir_##name

/*
 * Struct Types
 */
__RUNTIME_ATTR
Type *MEMOIR_FUNC(define_struct_type)(const char *name, int num_fields, ...);
__RUNTIME_ATTR
Type *MEMOIR_FUNC(struct_type)(const char *name);

/*
 * Static-length Tensor Type
 */
__RUNTIME_ATTR
Type *MEMOIR_FUNC(static_tensor_type)(Type *element_type,
                                      uint64_t num_dimensions,
                                      ...);

/*
 * Collection Types
 */
__RUNTIME_ATTR
Type *MEMOIR_FUNC(tensor_type)(Type *element_type, uint64_t num_dimensions);
__RUNTIME_ATTR
Type *MEMOIR_FUNC(assoc_array_type)(Type *key_type, Type *value_type);
__RUNTIME_ATTR
Type *MEMOIR_FUNC(sequence_type)(Type *element_type);

/*
 * Reference Type
 */
__RUNTIME_ATTR
Type *MEMOIR_FUNC(ref_type)(Type *referenced_type);

/*
 * Primitive Types
 */
__RUNTIME_ATTR
Type *MEMOIR_FUNC(u64_type)();
__RUNTIME_ATTR
Type *MEMOIR_FUNC(u32_type)();
__RUNTIME_ATTR
Type *MEMOIR_FUNC(u16_type)();
__RUNTIME_ATTR
Type *MEMOIR_FUNC(u8_type)();
__RUNTIME_ATTR
Type *MEMOIR_FUNC(i64_type)();
__RUNTIME_ATTR
Type *MEMOIR_FUNC(i32_type)();
__RUNTIME_ATTR
Type *MEMOIR_FUNC(i16_type)();
__RUNTIME_ATTR
Type *MEMOIR_FUNC(i8_type)();
__RUNTIME_ATTR
Type *MEMOIR_FUNC(bool_type)();
__RUNTIME_ATTR
Type *MEMOIR_FUNC(f32_type)();
__RUNTIME_ATTR
Type *MEMOIR_FUNC(f64_type)();
__RUNTIME_ATTR
Type *MEMOIR_FUNC(ptr_type)();

/*
 * Object construction
 */
__ALLOC_ATTR
__RUNTIME_ATTR
Object *MEMOIR_FUNC(allocate_struct)(Type *type);
__ALLOC_ATTR
__RUNTIME_ATTR
Object *MEMOIR_FUNC(allocate_tensor)(Type *element_type,
                                     uint64_t num_dimensions,
                                     ...);
__ALLOC_ATTR
__RUNTIME_ATTR
Object *MEMOIR_FUNC(allocate_assoc_array)(Type *key_type, Type *value_type);
__ALLOC_ATTR
__RUNTIME_ATTR
Object *MEMOIR_FUNC(allocate_sequence)(Type *element_type,
                                       uint64_t initial_size);

/*
 * Object destruction
 */
__RUNTIME_ATTR
void MEMOIR_FUNC(delete_object)(Object *object);

/*
 * Collection operations
 */
__RUNTIME_ATTR
Object *MEMOIR_FUNC(get_slice)(Object *object_to_slice, ...);
__RUNTIME_ATTR
Object *MEMOIR_FUNC(join)(
    uint8_t number_of_objects, // inclusive of the object_to_join
    Object *object_to_join,
    ...);

/*
 * Type checking and function signatures
 */
__RUNTIME_ATTR
bool MEMOIR_FUNC(assert_type)(Type *type, Object *object);
__RUNTIME_ATTR
bool MEMOIR_FUNC(set_return_type)(Type *type);

/*
 * Element accesses
 */
// Unsigned integer access
__RUNTIME_ATTR
void MEMOIR_FUNC(write_u64)(uint64_t value, Object *object_to_access, ...);
__RUNTIME_ATTR
void MEMOIR_FUNC(write_u32)(uint32_t value, Object *object_to_access, ...);
__RUNTIME_ATTR
void MEMOIR_FUNC(write_u16)(uint16_t value, Object *object_to_access, ...);
__RUNTIME_ATTR
void MEMOIR_FUNC(write_u8)(uint8_t value, Object *object_to_access, ...);
__RUNTIME_ATTR
uint64_t MEMOIR_FUNC(read_u64)(Object *object_to_access, ...);
__RUNTIME_ATTR
uint32_t MEMOIR_FUNC(read_u32)(Object *object_to_access, ...);
__RUNTIME_ATTR
uint16_t MEMOIR_FUNC(read_u16)(Object *object_to_access, ...);
__RUNTIME_ATTR
uint8_t MEMOIR_FUNC(read_u8)(Object *object_to_access, ...);

// Signed integer access
__RUNTIME_ATTR
void MEMOIR_FUNC(write_i64)(int64_t value, Object *object_to_access, ...);
__RUNTIME_ATTR
void MEMOIR_FUNC(write_i32)(int32_t value, Object *object_to_access, ...);
__RUNTIME_ATTR
void MEMOIR_FUNC(write_i16)(int16_t value, Object *object_to_access, ...);
__RUNTIME_ATTR
void MEMOIR_FUNC(write_i8)(int8_t value, Object *object_to_access, ...);
__RUNTIME_ATTR
int64_t MEMOIR_FUNC(read_i64)(Object *object_to_access, ...);
__RUNTIME_ATTR
int32_t MEMOIR_FUNC(read_i32)(Object *object_to_access, ...);
__RUNTIME_ATTR
int16_t MEMOIR_FUNC(read_i16)(Object *object_to_access, ...);
__RUNTIME_ATTR
int8_t MEMOIR_FUNC(read_i8)(Object *object_to_access, ...);

// Boolean access
__RUNTIME_ATTR
void MEMOIR_FUNC(write_bool)(bool value, Object *object_to_access, ...);
__RUNTIME_ATTR
bool MEMOIR_FUNC(read_bool)(Object *object_to_access, ...);

// Floating point access
__RUNTIME_ATTR
void MEMOIR_FUNC(write_f64)(double value, Object *object_to_access, ...);
__RUNTIME_ATTR
void MEMOIR_FUNC(write_f32)(float value, Object *object_to_access, ...);
__RUNTIME_ATTR
double MEMOIR_FUNC(read_f64)(Object *object_to_access, ...);
__RUNTIME_ATTR
float MEMOIR_FUNC(read_f32)(Object *object_to_access, ...);

// Reference access
__RUNTIME_ATTR
void MEMOIR_FUNC(write_ref)(Object *object_to_reference,
                            Object *object_to_access,
                            ...);
__RUNTIME_ATTR
Object *MEMOIR_FUNC(read_ref)(Object *object_to_access, ...);

// C Pointer access, these may NOT reference memoir objects
__RUNTIME_ATTR
void MEMOIR_FUNC(write_ptr)(void *pointer, Object *object_to_access, ...);
__RUNTIME_ATTR
void *MEMOIR_FUNC(read_ptr)(Object *object_to_access, ...);

// Nested object access
__RUNTIME_ATTR
Object *MEMOIR_FUNC(get_object)(Object *object_to_access, ...);

} // extern "C"
} // namespace memoir

#endif
