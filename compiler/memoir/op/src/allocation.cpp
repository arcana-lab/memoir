#include <iostream>
#include <stdarg.h>

#include "internal.h"
#include "memoir.h"

namespace memoir {

extern "C" {

// Allocation.
__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(allocate)(const type_ref type, ...) {
  collection_ref x;
  return x;
}

} // extern "C"

} // namespace memoir
