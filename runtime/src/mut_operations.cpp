/*
 * Object representation recognizable by LLVM IR
 * This file contains the implementation of the
 * mutable collections library.
 *
 * Author(s): Tommy McMichen
 * Created: July 12, 2023
 */

#include "internal.h"
#include "memoir.h"
#include "utils.h"

#include "objects.h"
#include "types.h"

namespace memoir {
extern "C" {

// Mutable collection operations.
__RUNTIME_ATTR
void MUT_FUNC(clear)(collection_ref collection, ...) {
  MEMOIR_ACCESS_CHECK(collection);
  auto *c = (detail::Collection *)(collection);
  MEMOIR_UNREACHABLE("Library implementation of clear is unimplemented."
                     "Please use the compiler.");
}

__RUNTIME_ATTR
void MUT_FUNC(insert)(collection_ref collection, ...) {
  MEMOIR_ACCESS_CHECK(collection);
  auto *c = (detail::Collection *)(collection);
  MEMOIR_UNREACHABLE("Library implementation of insert is unimplemented."
                     "Please use the compiler.");
}

__RUNTIME_ATTR
void MUT_FUNC(remove)(collection_ref collection, ...) {
  MEMOIR_ACCESS_CHECK(collection);
  auto *c = (detail::Collection *)(collection);
  MEMOIR_UNREACHABLE("Library implementation of remove is unimplemented."
                     "Please use the compiler.");
}

} // extern "C"
} // namespace memoir
