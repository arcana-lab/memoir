/*
 * Object representation recognizable by LLVM IR
 * This file contains the implementation of the
 * SSA use/def PHI operations.
 *
 * Author(s): Tommy McMichen
 * Created: August 3, 2023
 */

#include "internal.h"
#include "memoir.h"
#include "utils.h"

namespace memoir {

// General-purpose renaming operations.
__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(usePHI)(const collection_ref in) {
  return in;
}

__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(retPHI)(const collection_ref in, void *function) {
  return in;
}

// General-purpose operations.
__IMMUT_ATTR
__RUNTIME_ATTR
size_t MEMOIR_FUNC(size)(const collection_ref collection, ...) {
  MEMOIR_UNREACHABLE(
      "The MEMOIR library is deprecated. Please use the compiler.");
}

__IMMUT_ATTR
__RUNTIME_ATTR
size_t MEMOIR_FUNC(end)() {
  return -1;
}

// Fold operation.
#define HANDLE_TYPE(TYPE_NAME, C_TYPE)                                         \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(fold_##TYPE_NAME)(void *fold_function,                    \
                                       C_TYPE initial_value,                   \
                                       const collection_ref collection,        \
                                       ...) {                                  \
    MEMOIR_ASSERT(                                                             \
        false,                                                                 \
        "Fold is unimplemented in the library! Please use the compiler");      \
    return initial_value;                                                      \
  }                                                                            \
  __RUNTIME_ATTR                                                               \
  C_TYPE MEMOIR_FUNC(rfold_##TYPE_NAME)(void *fold_function,                   \
                                        C_TYPE initial_value,                  \
                                        const collection_ref collection,       \
                                        ...) {                                 \
    MEMOIR_ASSERT(                                                             \
        false,                                                                 \
        "Fold is unimplemented in the library! Please use the compiler");      \
    return initial_value;                                                      \
  }
#include "types.def"

__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(clear)(const collection_ref collection, ...) {
  MEMOIR_UNREACHABLE(
      "The MEMOIR library is deprecated. Please use the compiler.");
}

// Sequence operations.
__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(copy)(const collection_ref collection, ...) {
  MEMOIR_UNREACHABLE(
      "The MEMOIR library is deprecated. Please use the compiler.");
}

__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(remove)(const collection_ref collection, ...) {
  MEMOIR_UNREACHABLE(
      "The MEMOIR library is deprecated. Please use the compiler.");
}

__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(insert)(const collection_ref collection, ...) {
  MEMOIR_UNREACHABLE(
      "The MEMOIR library is deprecated. Please use the compiler.");
}

// Assoc operations.
__IMMUT_ATTR
__RUNTIME_ATTR
bool MEMOIR_FUNC(has)(const collection_ref collection, ...) {
  MEMOIR_ACCESS_CHECK(collection);

  va_list args;

  va_start(args, collection);

  auto result = ((detail::Collection *)collection)->has_element(args);

  va_end(args);

  return result;
}

__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(keys)(const collection_ref collection) {
  MEMOIR_ACCESS_CHECK(collection);

  MEMOIR_TYPE_CHECK(collection, TypeCode::AssocArrayTy);

  auto *assoc = (detail::AssocArray *)(collection);

  return (collection_ref)assoc->keys();
}

} // namespace memoir
