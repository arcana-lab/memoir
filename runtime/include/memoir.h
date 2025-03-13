#ifndef MEMOIR_MEMOIR_H
#define MEMOIR_MEMOIR_H

/*
 * Object representation recognizable by LLVM IR
 * This file describes the API for building and
 * accessing object-ir objects, fields and types
 *
 * Author(s): Tommy McMichen
 * Created: Mar 4, 2022
 */

#include <cstdarg>
#include <cstddef>
#include <cstdint>

#include "utils.h"

namespace memoir {

struct Type;
struct Collection;

extern "C" {

#define __RUNTIME_ATTR                                                         \
  extern "C" __declspec(noalias) __attribute__((nothrow))                      \
  __attribute__((noinline)) __attribute__((optnone)) __attribute__((used))
#define __ALLOC_ATTR __declspec(allocator)
#define __IMMUT_ATTR __attribute__((pure))

typedef Type *__restrict__ type_ref;
typedef Collection *__restrict__ collection_ref;
}

#define MEMOIR_FUNC(name) memoir__##name
#define MUT_FUNC(name) mut__##name

// Named Type
#if 0
__RUNTIME_ATTR
type_ref MEMOIR_FUNC(define_type)(const char *name, const type_ref type);

__RUNTIME_ATTR
type_ref MEMOIR_FUNC(lookup_type)(const char *name);
#endif

// Tuple Types
__RUNTIME_ATTR
type_ref MEMOIR_FUNC(tuple_type)(const type_ref first, ...);

// Static-length Array Type
__RUNTIME_ATTR
type_ref MEMOIR_FUNC(array_type)(const type_ref element_type, size_t length);

// Collection Types
__RUNTIME_ATTR
type_ref MEMOIR_FUNC(assoc_type)(const type_ref key_type,
                                 const type_ref value_type,
                                 ...);

__RUNTIME_ATTR
type_ref MEMOIR_FUNC(sequence_type)(const type_ref element_type, ...);

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

__RUNTIME_ATTR
type_ref MEMOIR_FUNC(void_type)();

// Object construction
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(allocate)(const type_ref type, ...);

// Object destruction.
__RUNTIME_ATTR
void MEMOIR_FUNC(delete)(collection_ref object);

// Collection operations.
__IMMUT_ATTR
__RUNTIME_ATTR
size_t MEMOIR_FUNC(size)(const collection_ref collection, ...);

__IMMUT_ATTR
__RUNTIME_ATTR
size_t MEMOIR_FUNC(end)();

__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(clear)(const collection_ref collection, ...);

#define HANDLE_TYPE(TYPE_NAME, C_TYPE)                                         \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(fold_##TYPE_NAME)(                                        \
      void *fold_function, /*C_TYPE (*f)(C_TYPE, ...),*/                       \
      C_TYPE initial_value,                                                    \
      const collection_ref collection,                                         \
      ...);                                                                    \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(rfold_##TYPE_NAME)(                                       \
      void *fold_function, /*C_TYPE (*f)(C_TYPE, ...),*/                       \
      C_TYPE initial_value,                                                    \
      const collection_ref collection,                                         \
      ...);
#include "types.def"

__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(copy)(const collection_ref collection, ...);

__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(insert)(const collection_ref collection, ...);

__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(remove)(const collection_ref collection, ...);

// Mutable collection operations.
__RUNTIME_ATTR
void MUT_FUNC(clear)(collection_ref collection, ...);

__RUNTIME_ATTR
void MUT_FUNC(insert)(collection_ref collection, ...);

__RUNTIME_ATTR
void MUT_FUNC(remove)(collection_ref collection, ...);

// SSA Assoc operations.
__IMMUT_ATTR
__RUNTIME_ATTR
bool MEMOIR_FUNC(has)(const collection_ref collection, ...);

__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(keys)(const collection_ref collection);

// Read/write accesses
#define HANDLE_TYPE(TYPE_NAME, C_TYPE)                                         \
  /* SSA Access */                                                             \
  __IMMUT_ATTR                                                                 \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(                                                          \
      read_##TYPE_NAME)(const collection_ref collection_to_access, ...);       \
                                                                               \
  __IMMUT_ATTR                                                                 \
  __ALLOC_ATTR                                                                 \
  __RUNTIME_ATTR                                                               \
  collection_ref MEMOIR_FUNC(write_##TYPE_NAME)(                               \
      C_TYPE value,                                                            \
      const collection_ref collection_to_access,                               \
      ...);                                                                    \
                                                                               \
  /* Mutable access */                                                         \
  __RUNTIME_ATTR                                                               \
  void MUT_FUNC(write_##TYPE_NAME)(C_TYPE value,                               \
                                   collection_ref collection_to_access,        \
                                   ...);
#include "types.def"

// Nested object access
__IMMUT_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(get)(const collection_ref collection_to_access, ...);

// SSA renaming
__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(usePHI)(const collection_ref collection);

__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(retPHI)(const collection_ref collection,
                                   void *function);

// Type checking and function signatures.
__RUNTIME_ATTR
bool MEMOIR_FUNC(assert_type)(const type_ref type, const collection_ref object);
__RUNTIME_ATTR
void MEMOIR_FUNC(return_type)(const type_ref type);

// MEMOIR Keyword arguments.
#define MEMOIR_KEYWORD(NAME) "memoir." #NAME
__attribute__((used, weak))
const char *MEMOIR_FUNC(keywords)[] = { MEMOIR_KEYWORD(closed),
                                        MEMOIR_KEYWORD(range),
                                        MEMOIR_KEYWORD(input),
                                        MEMOIR_KEYWORD(value),
                                        MEMOIR_KEYWORD(selection) };

} // namespace memoir

#endif
