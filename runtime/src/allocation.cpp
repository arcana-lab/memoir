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
struct_ref MEMOIR_FUNC(allocate_struct)(const type_ref type) {
  auto strct = new struct Struct(type);

  return strct;
}

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

  auto tensor = new struct Tensor(tensor_type, length_of_dimensions);

  return tensor;
}

__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(allocate_assoc_array)(const type_ref key_type,
                                                 const type_ref value_type) {
  auto assoc_array_type = AssocArrayType::get(key_type, value_type);

  auto assoc_array = new struct AssocArray(assoc_array_type);

  return assoc_array;
}

__IMMUT_ATTR
__ALLOC_ATTR
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(allocate_sequence)(const type_ref element_type,
                                              uint64_t init_size) {
  auto sequence_type = SequenceType::get(element_type);

  auto sequence = new struct SequenceAlloc(sequence_type, init_size);

  return sequence;
}

// DEPRECATED
__RUNTIME_ATTR
collection_ref MEMOIR_FUNC(sequence_view)(
    const collection_ref collection_to_view,
    size_t begin,
    size_t end) {
  MEMOIR_ASSERT((collection_to_view != nullptr), "Attempt to view NULL object");
  if (auto *seq = dynamic_cast<SequenceAlloc *>(collection_to_view)) {
    return new SequenceView(seq, begin, end);
  } else if (auto *seq_view =
                 dynamic_cast<SequenceView *>(collection_to_view)) {
    return new SequenceView(seq_view, begin, end);
  }
  MEMOIR_UNREACHABLE("Attempt to view a non-viewable object");
}

} // extern "C"

} // namespace memoir
