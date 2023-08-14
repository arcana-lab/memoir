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
extern "C" {

__IMMUT_ATTR
__RUNTIME_ATTR
Collection *MEMOIR_FUNC(defPHI)(Collection *in) {
  return in;
}

__IMMUT_ATTR
__RUNTIME_ATTR
Collection *MEMOIR_FUNC(usePHI)(Collection *in) {
  return in;
}
}
} // namespace memoir
