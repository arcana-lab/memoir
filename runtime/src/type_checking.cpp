/*
 * Object representation recognizable by LLVM IR
 * This file contains methods to access object-ir
 * objects, fields and types for the object-ir library.
 *
 * Author(s): Tommy McMichen
 * Created: Mar 17, 2022
 */

#include <cassert>
#include <iostream>

#include "internal.h"
#include "memoir.h"

namespace memoir {

extern "C" {

// Type checking
__RUNTIME_ATTR
bool MEMOIR_FUNC(assert_struct_type)(const type_ref type,
                                     const struct_ref object) {
  if (object == nullptr) {
    return is_object_type(type);
  }

  MEMOIR_ASSERT((type->equals(((detail::Object *)object)->get_type())),
                "Struct is not the correct type");

  return true;
}

__RUNTIME_ATTR
bool MEMOIR_FUNC(assert_collection_type)(const type_ref type,
                                         const collection_ref object) {
  if (object == nullptr) {
    return is_object_type(type);
  }

  MEMOIR_ASSERT((type->equals(((detail::Object *)object)->get_type())),
                "Collection is not the correct type");

  return true;
}

__RUNTIME_ATTR
bool MEMOIR_FUNC(set_return_type)(const type_ref type) {
  return true;
}

__RUNTIME_ATTR
void MEMOIR_FUNC(property)(const char *property_id, ...) {
  return;
}

} // extern "C"

} // namespace memoir
