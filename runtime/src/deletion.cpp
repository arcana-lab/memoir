#include "internal.h"
#include "memoir.h"

#include "objects.h"

namespace memoir {

extern "C" {

// Deletion
__RUNTIME_ATTR
void MEMOIR_FUNC(delete)(const collection_ref cllct) {
  MEMOIR_UNREACHABLE(
      "The MEMOIR runtime library is deprecated. Please use the compiler.");
}

} // extern "C"

} // namespace memoir
