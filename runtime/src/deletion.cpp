#include "internal.h"
#include "memoir.h"

namespace memoir {

extern "C" {

// Deletion
void MEMOIR_FUNC(delete_struct)(const struct_ref strct) {
  delete (detail::Struct *)strct;
}

void MEMOIR_FUNC(delete_collection)(const collection_ref cllct) {
  delete (detail::Collection *)cllct;
}

} // extern "C"

} // namespace memoir
