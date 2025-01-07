#include "memoir.h"

using namespace memoir;

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

__RUNTIME_ATTR
type_ref MEMOIR_FUNC(void_type)();

// Object construction
__IMMUT_ATTR
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
bool MEMOIR_FUNC(assoc_has)(const collection_ref collection, ...);

__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(assoc_keys)(const collection_ref collection);

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

// Nested object access
collection_ref MEMOIR_FUNC(get)(const collection_ref collection_to_access,
                                ...) {
  return collection;
}

// SSA renaming
__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(usePHI)(const collection_ref collection) {
  return collection;
}

__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(argPHI)(const collection_ref collection) {
  return collection;
}

__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(retPHI)(const collection_ref collection,
                                   void *function) {
  return collection;
}

// Type checking and function signatures.
__RUNTIME_ATTR
bool MEMOIR_FUNC(assert_type)(const type_ref type,
                              const collection_ref object) {
  return true;
}
__RUNTIME_ATTR
void MEMOIR_FUNC(return_type)(const type_ref type) {
  return true;
}
