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
      __attribute__((optnone)) __attribute__((used))
#define __ALLOC_ATTR __declspec(allocator)
#define __IMMUT_ATTR __attribute__((pure)) // __attribute__((const))

#define MEMOIR_FUNC(name) memoir__##name

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
#define HANDLE_INTEGER_TYPE(TYPE_NAME, C_TYPE, BITWIDTH, IS_SIGNED)            \
  __RUNTIME_ATTR                                                               \
  Type *MEMOIR_FUNC(TYPE_NAME##_type)();

#define HANDLE_PRIMITIVE_TYPE(TYPE_NAME, C_TYPE, CLASS_PREFIX)                 \
  __RUNTIME_ATTR                                                               \
  Type *MEMOIR_FUNC(TYPE_NAME##_type)();

/*
 * Object construction
 */
__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
Struct *MEMOIR_FUNC(allocate_struct)(Type *type);

__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
Collection *MEMOIR_FUNC(allocate_tensor)(Type *element_type,
                                         uint64_t num_dimensions,
                                         ...);

__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
Collection *MEMOIR_FUNC(allocate_assoc_array)(Type *key_type, Type *value_type);

__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
Collection *MEMOIR_FUNC(allocate_sequence)(Type *element_type,
                                           uint64_t initial_size);

/*
 * Object destruction
 */
__RUNTIME_ATTR
void MEMOIR_FUNC(delete_struct)(Struct *object);

__RUNTIME_ATTR
void MEMOIR_FUNC(delete_collection)(Collection *collection);

/*
 * Collection operations.
 */
__IMMUT_ATTR
__RUNTIME_ATTR
uint64_t MEMOIR_FUNC(size)(Collection *collection);

// Immutable sequence operations.
__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
Collection *MEMOIR_FUNC(get_slice)(Collection *collection_to_slice, ...);

__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
Collection *MEMOIR_FUNC(join)(uint8_t number_of_collections,
                              Collection *collection_to_join,
                              ...);

// Mutable sequence operations.
#define HANDLE_TYPE(TYPE_NAME, C_TYPE)                                         \
  __RUNTIME_ATTR                                                               \
  void MEMOIR_FUNC(sequence_insert_##TYPE_NAME)(C_TYPE value,                  \
                                                Collection * collection,       \
                                                size_t index);
#include "types.def"

__RUNTIME_ATTR
void MEMOIR_FUNC(sequence_insert)(Collection *collection_to_insert,
                                  Collection *collection,
                                  size_t insertion_index);

__RUNTIME_ATTR
void MEMOIR_FUNC(sequence_remove)(Collection *collection,
                                  size_t begin,
                                  size_t end);

__RUNTIME_ATTR
void MEMOIR_FUNC(sequence_append)(Collection *collection,
                                  Collection *collection_to_append);

__RUNTIME_ATTR
void MEMOIR_FUNC(sequence_swap)(Collection *collection,
                                size_t i,
                                size_t j,
                                Collection *collection2,
                                size_t i2);

__ALLOC_ATTR
__RUNTIME_ATTR
Collection *MEMOIR_FUNC(sequence_split)(Collection *collection,
                                        size_t i,
                                        size_t j);

__RUNTIME_ATTR
Collection *MEMOIR_FUNC(sequence_view)(Collection *collection,
                                       size_t,
                                       size_t j);

// Associative array operations.
__IMMUT_ATTR
__RUNTIME_ATTR
bool MEMOIR_FUNC(assoc_has)(Collection *collection, ...);

__RUNTIME_ATTR
void MEMOIR_FUNC(assoc_remove)(Collection *collection, ...);

__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
Collection *MEMOIR_FUNC(assoc_keys)(Collection *collection);

/*
 * Read/Write accesses
 */
#define HANDLE_TYPE(TYPE_NAME, C_TYPE)                                         \
  __IMMUT_ATTR                                                                 \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(struct_read_##TYPE_NAME)(Struct * struct_to_access,       \
                                              unsigned field_index);           \
  __IMMUT_ATTR                                                                 \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(                                                          \
      index_read_##TYPE_NAME)(Collection * collection_to_access, ...);         \
  __IMMUT_ATTR                                                                 \
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
  __IMMUT_ATTR                                                                 \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(struct_get_##TYPE_NAME)(Struct * struct_to_access,        \
                                             unsigned field_index);            \
  __IMMUT_ATTR                                                                 \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(index_get_##TYPE_NAME)(Collection * collection_to_access, \
                                            ...);                              \
  __IMMUT_ATTR                                                                 \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(assoc_get_##TYPE_NAME)(Collection * collection_to_access, \
                                            ...);
#include "types.def"

// SSA and readonce renaming
__IMMUT_ATTR
__RUNTIME_ATTR
Collection *MEMOIR_FUNC(defPHI)(Collection *collection);

__IMMUT_ATTR
__RUNTIME_ATTR
Collection *MEMOIR_FUNC(usePHI)(Collection *collection);

/*
 * Type checking and function signatures.
 */
__IMMUT_ATTR
__RUNTIME_ATTR
bool MEMOIR_FUNC(assert_struct_type)(Type *type, Struct *object);

__IMMUT_ATTR
__RUNTIME_ATTR
bool MEMOIR_FUNC(assert_collection_type)(Type *type, Collection *object);
__IMMUT_ATTR
__RUNTIME_ATTR
bool MEMOIR_FUNC(set_return_type)(Type *type);

} // extern "C"
} // namespace memoir

#endif
