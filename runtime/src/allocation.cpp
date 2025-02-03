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

/*
__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(allocate_tensor)(const type_ref element_type,
                                          uint64_t num_dimensions,
                                          ...) {
std::vector<uint64_t> length_of_dimensions;

va_list args;

va_start(args, num_dimensions);

for (int i = 0; i < num_dimensions; i++) {
  auto length_of_dimension = va_arg(args, uint64_t);
  length_of_dimensions.push_back(length_of_dimension);
}

va_end(args);

auto tensor_type = TensorType::get(element_type, num_dimensions);

auto tensor = new struct detail::Tensor(tensor_type, length_of_dimensions);

return (collection_ref)tensor;
}

__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(allocate_assoc_array)(const type_ref key_type,
                                               const type_ref value_type) {
auto assoc_array_type = AssocArrayType::get(key_type, value_type);

auto assoc_array = new struct detail::AssocArray(assoc_array_type);

return (collection_ref)assoc_array;
}

__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(allocate_sequence)(const type_ref element_type,
                                            uint64_t init_size) {
auto sequence_type = SequenceType::get(element_type);

auto sequence = new struct detail::SequenceAlloc(sequence_type, init_size);

return (collection_ref)sequence;
}
*/

} // extern "C"

} // namespace memoir
