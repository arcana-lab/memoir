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

#include <stdarg.h>

#include "objects.h"
#include "types.h"
#include "utils.h"

namespace memoir {
extern "C" {

#define __RUNTIME_ATTR                                                         \
  __declspec(noalias) __attribute__((nothrow)) __attribute__((noinline))       \
      __attribute__((optnone)) __attribute__((used))
#define __ALLOC_ATTR __declspec(allocator)
#define __IMMUT_ATTR __attribute__((pure))

typedef Type *__restrict__ type_ref;
typedef Collection *__restrict__ collection_ref;
typedef Struct *__restrict__ struct_ref;

typedef struct {
  collection_ref first;
  collection_ref second;
} collection_pair;

#define MEMOIR_FUNC(name) memoir__##name
#define MUT_FUNC(name) mut__##name

// Struct Types
__RUNTIME_ATTR type_ref MEMOIR_FUNC(define_struct_type)(const char *name,
                                                        int num_fields,
                                                        ...);

__RUNTIME_ATTR
type_ref MEMOIR_FUNC(struct_type)(const char *name);

// Static-length Tensor Type
__RUNTIME_ATTR
type_ref MEMOIR_FUNC(static_tensor_type)(const type_ref element_type,
                                         uint64_t num_dimensions,
                                         ...);
// Collection Types
__RUNTIME_ATTR
type_ref MEMOIR_FUNC(tensor_type)(const type_ref element_type,
                                  uint64_t num_dimensions);

__RUNTIME_ATTR
type_ref MEMOIR_FUNC(assoc_array_type)(const type_ref key_type,
                                       const type_ref value_type);

__RUNTIME_ATTR
type_ref MEMOIR_FUNC(sequence_type)(const type_ref element_type);

// Reference Type
__RUNTIME_ATTR
type_ref MEMOIR_FUNC(ref_type)(const type_ref referenced_type);

// Primitive Types
#define HANDLE_INTEGER_TYPE(TYPE_NAME, C_TYPE, BITWIDTH, IS_SIGNED)            \
  __RUNTIME_ATTR                                                               \
  type_ref MEMOIR_FUNC(TYPE_NAME##_type)();

#define HANDLE_PRIMITIVE_TYPE(TYPE_NAME, C_TYPE, CLASS_PREFIX)                 \
  __RUNTIME_ATTR                                                               \
  type_ref MEMOIR_FUNC(TYPE_NAME##_type)();

// Object construction
__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
struct_ref MEMOIR_FUNC(allocate_struct)(const type_ref type);

__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(allocate_tensor)(const type_ref element_type,
                                            uint64_t num_dimensions,
                                            ...);

__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(allocate_assoc_array)(const type_ref key_type,
                                                 const type_ref value_type);

__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(allocate_sequence)(const type_ref element_type,
                                              uint64_t initial_size);

// Field arrays.
__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(field_array)(const type_ref struct_type,
                                        unsigned field_index);

// Object destruction.
__RUNTIME_ATTR
void MEMOIR_FUNC(delete_struct)(struct_ref object);

__RUNTIME_ATTR
void MEMOIR_FUNC(delete_collection)(collection_ref collection);

// Collection operations.
__IMMUT_ATTR
__RUNTIME_ATTR
size_t MEMOIR_FUNC(size)(const collection_ref collection);

__IMMUT_ATTR
__RUNTIME_ATTR
size_t MEMOIR_FUNC(end)();

// Immutable sequence operations.
__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(sequence_copy)(collection_ref collection,
                                          size_t begin_index,
                                          size_t end_index);

#define HANDLE_TYPE(TYPE_NAME, C_TYPE)                                         \
  __IMMUT_ATTR                                                                 \
  __ALLOC_ATTR                                                                 \
  __RUNTIME_ATTR                                                               \
  collection_ref MEMOIR_FUNC(sequence_insert_##TYPE_NAME)(                     \
      C_TYPE value,                                                            \
      const collection_ref collection,                                         \
      size_t insertion_index);
#include "types.def"

__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(sequence_insert)(
    const collection_ref collection_to_insert,
    const collection_ref collection,
    size_t insertion_index);

__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(sequence_remove)(const collection_ref collection,
                                            size_t begin_index,
                                            size_t end_index);

__IMMUT_ATTR
__RUNTIME_ATTR
const collection_pair MEMOIR_FUNC(sequence_swap)(
    const collection_ref collection1,
    size_t begin1,
    size_t end1,
    const collection_ref collection2,
    size_t begin2);

__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(sequence_swap_within)(
    const collection_ref collection,
    size_t from_begin,
    size_t from_end,
    size_t to_begin);

// Mutable sequence operations.
#define HANDLE_TYPE(TYPE_NAME, C_TYPE)                                         \
  __RUNTIME_ATTR                                                               \
  void MUT_FUNC(sequence_insert_##TYPE_NAME)(C_TYPE value,                     \
                                             collection_ref collection,        \
                                             size_t index);
#include "types.def"

__RUNTIME_ATTR
void MUT_FUNC(sequence_insert)(const collection_ref collection_to_insert,
                               collection_ref collection,
                               size_t insertion_index);

__RUNTIME_ATTR
void MUT_FUNC(sequence_remove)(collection_ref collection,
                               size_t begin,
                               size_t end);

__RUNTIME_ATTR
void MUT_FUNC(sequence_append)(collection_ref collection,
                               collection_ref collection_to_append);

__RUNTIME_ATTR
void MUT_FUNC(sequence_swap)(collection_ref collection,
                             size_t i,
                             size_t j,
                             collection_ref collection2,
                             size_t i2);

__RUNTIME_ATTR
void MUT_FUNC(sequence_swap_within)(collection_ref collection,
                                    size_t i,
                                    size_t j,
                                    size_t k);
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MUT_FUNC(sequence_split)(collection_ref collection,
                                        size_t i,
                                        size_t j);

// Lowering operations.
// Possibly deprecated, don't add any more uses of this.
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(sequence_view)(collection_ref collection,
                                          size_t i,
                                          size_t j);

// SSA Assoc operations.
__IMMUT_ATTR
__RUNTIME_ATTR
bool MEMOIR_FUNC(assoc_has)(const collection_ref collection, ...);

__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(assoc_insert)(const collection_ref collection, ...);

__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(assoc_remove)(const collection_ref collection, ...);

__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(assoc_keys)(const collection_ref collection);

// Mutable assoc operations
__RUNTIME_ATTR
void MUT_FUNC(assoc_insert)(collection_ref collection, ...);

__RUNTIME_ATTR
void MUT_FUNC(assoc_remove)(collection_ref collection, ...);

// Read/write accesses
#define HANDLE_TYPE(TYPE_NAME, C_TYPE)                                         \
  /* SSA Access */                                                             \
  __IMMUT_ATTR                                                                 \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(struct_read_##TYPE_NAME)(                                 \
      const struct_ref struct_to_access,                                       \
      unsigned field_index);                                                   \
  __IMMUT_ATTR                                                                 \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(                                                          \
      index_read_##TYPE_NAME)(const collection_ref collection_to_access, ...); \
  __IMMUT_ATTR                                                                 \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(                                                          \
      assoc_read_##TYPE_NAME)(const collection_ref collection_to_access, ...); \
  __RUNTIME_ATTR                                                               \
  void MEMOIR_FUNC(struct_write_##TYPE_NAME)(C_TYPE value,                     \
                                             struct_ref struct_to_access,      \
                                             unsigned field_index);            \
  __IMMUT_ATTR                                                                 \
  __ALLOC_ATTR                                                                 \
  __RUNTIME_ATTR                                                               \
  collection_ref MEMOIR_FUNC(index_write_##TYPE_NAME)(                         \
      C_TYPE value,                                                            \
      const collection_ref collection_to_access,                               \
      ...);                                                                    \
  __IMMUT_ATTR                                                                 \
  __ALLOC_ATTR                                                                 \
  __RUNTIME_ATTR                                                               \
  collection_ref MEMOIR_FUNC(assoc_write_##TYPE_NAME)(                         \
      C_TYPE value,                                                            \
      const collection_ref collection_to_access,                               \
      ...);                                                                    \
  /* Mutable access */                                                         \
  __RUNTIME_ATTR                                                               \
  void MUT_FUNC(struct_write_##TYPE_NAME)(C_TYPE value,                        \
                                          Struct * struct_to_access,           \
                                          unsigned field_index);               \
  __RUNTIME_ATTR                                                               \
  void MUT_FUNC(index_write_##TYPE_NAME)(C_TYPE value,                         \
                                         collection_ref collection_to_access,  \
                                         ...);                                 \
  __RUNTIME_ATTR                                                               \
  void MUT_FUNC(assoc_write_##TYPE_NAME)(C_TYPE value,                         \
                                         collection_ref collection_to_access,  \
                                         ...);

// Nested object access
#define HANDLE_NESTED_TYPE(TYPE_NAME, C_TYPE, CLASS_PREFIX)                    \
  __IMMUT_ATTR                                                                 \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(struct_get_##TYPE_NAME)(                                  \
      const struct_ref struct_to_access,                                       \
      unsigned field_index);                                                   \
  __IMMUT_ATTR                                                                 \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(                                                          \
      index_get_##TYPE_NAME)(const collection_ref collection_to_access, ...);  \
  __IMMUT_ATTR                                                                 \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(                                                          \
      assoc_get_##TYPE_NAME)(const collection_ref collection_to_access, ...);
#include "types.def"

// SSA renaming
__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(defPHI)(const collection_ref collection);

__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(usePHI)(const collection_ref collection);

__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(argPHI)(const collection_ref collection);

__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(retPHI)(const collection_ref collection);

// Type checking and function signatures.
__RUNTIME_ATTR
bool MEMOIR_FUNC(assert_struct_type)(const type_ref type,
                                     const struct_ref object);
__RUNTIME_ATTR
bool MEMOIR_FUNC(assert_collection_type)(const type_ref type,
                                         const collection_ref object);
__RUNTIME_ATTR
bool MEMOIR_FUNC(set_return_type)(const type_ref type);

} // extern "C"
} // namespace memoir

#endif
