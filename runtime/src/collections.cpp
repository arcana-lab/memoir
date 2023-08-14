#include <iostream>

#include "internal.h"
#include "objects.h"

namespace memoir {

/*
 * Collection Objects
 */
Collection::Collection(Type *type) : Object(type) {
  // Do nothing.
}

bool Collection::is_collection() const {
  return true;
}

/*
 * Tensor Objects
 */
Tensor::Tensor(Type *type) : Collection(type) {
  MEMOIR_ASSERT((this->type->getCode() == TypeCode::TensorTy),
                "Trying to create a tensor of non-tensor type");

  auto tensor_type = (TensorType *)(this->type);
  auto element_type = tensor_type->element_type;
  auto num_dimensions = tensor_type->num_dimensions;
  this->length_of_dimensions = tensor_type->length_of_dimensions;

  MEMOIR_ASSERT((num_dimensions > 0),
                "Trying to create a tensor with 0 dimensions");

  // FIXME
  // Initialize the tensor
  // for (auto dimension_length : this->length_of_dimensions) {
  //   for (auto i = 0; i < dimension_length; i++) {
  //     auto field = Element::create(element_type);
  //     this->tensor.push_back(field);
  //   }
  // }
}

Tensor::Tensor(Type *type, std::vector<uint64_t> &length_of_dimensions)
  : Collection(type),
    length_of_dimensions(length_of_dimensions) {

  MEMOIR_ASSERT((this->type->getCode() == TypeCode::TensorTy),
                "Trying to create a tensor of non-tensor type");

  auto tensor_type = (TensorType *)(this->type);
  auto element_type = tensor_type->element_type;
  auto num_dimensions = tensor_type->num_dimensions;

  MEMOIR_ASSERT((num_dimensions > 0),
                "Trying to create a tensor with 0 dimensions");

  // Initialize the tensor
  auto flattened_length = 0;
  auto last_dimension_length = 1;
  for (auto i = 0; i < length_of_dimensions.size(); i++) {
    auto dimension_length = length_of_dimensions[i];

    flattened_length *= last_dimension_length;
    flattened_length += dimension_length;
    last_dimension_length = dimension_length;
  }

  // FIXME
  auto init_vector = init_elements(element_type, flattened_length);
  this->tensor.resize(init_vector.size());
  std::move(init_vector.begin(), init_vector.end(), this->tensor.begin());
}

void Tensor::free() {
  auto *element_type = this->get_element_type();
  if (is_object_type(element_type)) {
    for (auto it = this->tensor.begin(); it != this->tensor.end(); ++it) {
      ((Object *)(*it))->free();
    }
  }
}

uint64_t Tensor::get_tensor_element(std::vector<uint64_t> &indices) const {
  auto flattened_index = 0;
  auto last_dimension_length = 1;
  for (auto i = 0; i < this->length_of_dimensions.size(); i++) {
    auto dimension_length = this->length_of_dimensions[i];
    auto index = indices[i];
    MEMOIR_ASSERT((index < dimension_length),
                  "Index out of range for tensor access");

    flattened_index *= last_dimension_length;
    flattened_index += index;
    last_dimension_length = dimension_length;
  }

  return this->tensor[flattened_index];
}

uint64_t Tensor::get_element(va_list args) {
  auto num_dimensions = this->length_of_dimensions.size();

  std::vector<uint64_t> indices = {};
  for (auto i = 0; i < num_dimensions; i++) {
    auto index = va_arg(args, uint64_t);
    indices.push_back(index);
  }

  return this->get_tensor_element(indices);
}

void Tensor::set_tensor_element(uint64_t value,
                                std::vector<uint64_t> &indices) {
  auto flattened_index = 0;
  auto last_dimension_length = 1;
  for (auto i = 0; i < this->length_of_dimensions.size(); i++) {
    auto dimension_length = this->length_of_dimensions[i];
    auto index = indices[i];
    MEMOIR_ASSERT((index < dimension_length),
                  "Index out of range for tensor access");

    flattened_index *= last_dimension_length;
    flattened_index += index;
    last_dimension_length = dimension_length;
  }

  this->tensor[flattened_index] = value;
}

void Tensor::set_element(uint64_t value, va_list args) {
  auto num_dimensions = this->length_of_dimensions.size();

  std::vector<uint64_t> indices = {};
  for (auto i = 0; i < num_dimensions; i++) {
    auto index = va_arg(args, uint64_t);
    indices.push_back(index);
  }

  this->set_tensor_element(value, indices);
}

bool Tensor::has_tensor_element(std::vector<uint64_t> &indices) const {
  auto flattened_index = 0;
  auto last_dimension_length = 1;
  for (auto i = 0; i < this->length_of_dimensions.size(); i++) {
    auto dimension_length = this->length_of_dimensions[i];
    auto index = indices[i];
    MEMOIR_ASSERT((index < dimension_length),
                  "Index out of range for tensor access");

    flattened_index *= last_dimension_length;
    flattened_index += index;
    last_dimension_length = dimension_length;
  }

  // TODO.
  return true;
}

bool Tensor::has_element(va_list args) {
  auto num_dimensions = this->length_of_dimensions.size();

  std::vector<uint64_t> indices = {};
  for (auto i = 0; i < num_dimensions; i++) {
    auto index = va_arg(args, uint64_t);
    indices.push_back(index);
  }

  return this->has_tensor_element(indices);
}

void Tensor::remove_element(va_list args) {
  auto num_dimensions = this->length_of_dimensions.size();

  std::vector<uint64_t> indices = {};
  for (auto i = 0; i < num_dimensions; i++) {
    auto index = va_arg(args, uint64_t);
    indices.push_back(index);
  }

  return;
}

Collection *Tensor::get_slice(va_list args) {
  MEMOIR_UNREACHABLE("Attempt to get slice of tensor, UNSUPPORTED");

  return nullptr;
}

Collection *Tensor::join(va_list args, uint8_t num_args) {
  MEMOIR_UNREACHABLE(
      "Attempt to perfrom join operation on tensor, UNSUPPORTED");

  return nullptr;
}

Type *Tensor::get_element_type() const {
  auto tensor_type = static_cast<TensorType *>(this->get_type());

  return tensor_type->element_type;
}

uint64_t Tensor::size() const {
  // TODO
  return 0;
}

bool Tensor::equals(const Object *other) const {
  return (this == other);
}

/*
 * Associative Array implementation
 */
AssocArray::AssocArray(Type *type) : Collection(type) {
  MEMOIR_ASSERT(
      (type->getCode() == TypeCode::AssocArrayTy),
      "Trying to create an associative array of non-associative array type\n");

  this->assoc_array.clear();
}

void AssocArray::free() {
  auto *element_type = this->get_element_type();
  if (is_object_type(element_type)) {
    for (auto it = this->assoc_array.begin(); it != this->assoc_array.end();
         ++it) {
      ((Object *)(it->second))->free();
    }
  }
}

uint64_t AssocArray::get_element(va_list args) {
  key_t key;

  auto key_type = this->get_key_type();
  switch (key_type->getCode()) {
    case TypeCode::StructTy: {
      key = (key_t)va_arg(args, Object *);
      // TODO: need special handling to check for field equality
      break;
    }
    case TypeCode::TensorTy: {
      key = (key_t)va_arg(args, Object *);
      // TODO: need special handling to check for element equality
      break;
    }
    case TypeCode::AssocArrayTy: {
      key = (key_t)va_arg(args, Object *);
      // TODO: need special handling to check for element equality
      break;
    }
    case TypeCode::SequenceTy: {
      key = (key_t)va_arg(args, Object *);
      // TODO: need special handling to check for element equality
      break;
    }
    case TypeCode::IntegerTy: {
      auto integer_type = static_cast<IntegerType *>(key_type);
      auto bitwidth = integer_type->bitwidth;
      if (bitwidth < 32) {
        key = (key_t)((integer_type->is_signed) ? va_arg(args, int)
                                                : va_arg(args, int));
      } else if (bitwidth <= 32) {
        key = (key_t)((integer_type->is_signed) ? va_arg(args, int32_t)
                                                : va_arg(args, uint32_t));
      } else if (bitwidth <= 64) {
        key = (key_t)((integer_type->is_signed) ? va_arg(args, int64_t)
                                                : va_arg(args, uint64_t));
      }
      break;
    }
    case TypeCode::FloatTy: {
      auto type = static_cast<FloatType *>(key_type);
      key = (key_t)((float)va_arg(args, double));
      break;
    }
    case TypeCode::DoubleTy: {
      auto type = static_cast<DoubleType *>(key_type);
      key = (key_t)va_arg(args, double);
      break;
    }
    case TypeCode::PointerTy: {
      auto type = static_cast<PointerType *>(key_type);
      key = (key_t)va_arg(args, void *);
      break;
    }
    case TypeCode::ReferenceTy: {
      auto type = static_cast<ReferenceType *>(key_type);
      key = (key_t)va_arg(args, Object *);
      break;
    }
    default: {
      MEMOIR_UNREACHABLE("Attempt to get element with unknown key type");
    }
  }

  auto found_key = this->assoc_array.find(key);
  if (found_key != this->assoc_array.end()) {
    return found_key->second;
  } else {
    // Create a new key-value pair and return it.
    auto new_elem = init_element(this->get_value_type());
    this->assoc_array[key] = new_elem;
    return new_elem;
  }
}

void AssocArray::set_element(uint64_t value, va_list args) {
  key_t key;

  auto key_type = this->get_key_type();
  switch (key_type->getCode()) {
    case TypeCode::StructTy: {
      key = (key_t)va_arg(args, Object *);
      break;
    }
    case TypeCode::TensorTy: {
      key = (key_t)va_arg(args, Object *);
      break;
    }
    case TypeCode::AssocArrayTy: {
      key = (key_t)va_arg(args, Object *);
      break;
    }
    case TypeCode::SequenceTy: {
      key = (key_t)va_arg(args, Object *);
      break;
    }
    case TypeCode::IntegerTy: {
      auto integer_type = static_cast<IntegerType *>(key_type);
      auto bitwidth = integer_type->bitwidth;
      if (bitwidth < 32) {
        key = (key_t)((integer_type->is_signed) ? va_arg(args, int)
                                                : va_arg(args, int));
      } else if (bitwidth <= 32) {
        key = (key_t)((integer_type->is_signed) ? va_arg(args, int32_t)
                                                : va_arg(args, uint32_t));
      } else if (bitwidth <= 64) {
        key = (key_t)((integer_type->is_signed) ? va_arg(args, int64_t)
                                                : va_arg(args, uint64_t));
      }
      break;
    }
    case TypeCode::FloatTy: {
      auto type = static_cast<FloatType *>(key_type);
      key = (key_t)((float)va_arg(args, double));
      break;
    }
    case TypeCode::DoubleTy: {
      auto type = static_cast<DoubleType *>(key_type);
      key = (key_t)va_arg(args, double);
      break;
    }
    case TypeCode::PointerTy: {
      auto type = static_cast<PointerType *>(key_type);
      key = (key_t)va_arg(args, void *);
      break;
    }
    case TypeCode::ReferenceTy: {
      auto type = static_cast<ReferenceType *>(key_type);
      key = (key_t)va_arg(args, Object *);
      break;
    }
    default: {
      MEMOIR_UNREACHABLE("Attempt to get element with unknown key type");
    }
  }

  // FIXME
  this->assoc_array[key] = (value_t)value;
  this->assoc_array.find(key);
}

bool AssocArray::has_element(va_list args) {
  key_t key;

  auto key_type = this->get_key_type();
  switch (key_type->getCode()) {
    case TypeCode::StructTy: {
      key = (key_t)va_arg(args, Object *);
      break;
    }
    case TypeCode::TensorTy: {
      key = (key_t)va_arg(args, Object *);
      break;
    }
    case TypeCode::AssocArrayTy: {
      key = (key_t)va_arg(args, Object *);
      break;
    }
    case TypeCode::SequenceTy: {
      key = (key_t)va_arg(args, Object *);
      break;
    }
    case TypeCode::IntegerTy: {
      auto integer_type = static_cast<IntegerType *>(key_type);
      auto bitwidth = integer_type->bitwidth;
      if (bitwidth < 32) {
        key = (key_t)((integer_type->is_signed) ? va_arg(args, int)
                                                : va_arg(args, int));
      } else if (bitwidth <= 32) {
        key = (key_t)((integer_type->is_signed) ? va_arg(args, int32_t)
                                                : va_arg(args, uint32_t));
      } else if (bitwidth <= 64) {
        key = (key_t)((integer_type->is_signed) ? va_arg(args, int64_t)
                                                : va_arg(args, uint64_t));
      }
      return (this->assoc_array.find(key) != this->assoc_array.end());
    }
    case TypeCode::FloatTy: {
      auto type = static_cast<FloatType *>(key_type);
      auto key_as_float = (float)va_arg(args, double);
      key = (key_t)key_as_float;
      break;
    }
    case TypeCode::DoubleTy: {
      auto type = static_cast<DoubleType *>(key_type);
      auto key_as_double = va_arg(args, double);
      key = (key_t)key_as_double;
      break;
    }
    case TypeCode::PointerTy: {
      auto type = static_cast<PointerType *>(key_type);
      auto key_as_ptr = va_arg(args, void *);
      key = (key_t)key_as_ptr;
      break;
    }
    case TypeCode::ReferenceTy: {
      auto type = static_cast<ReferenceType *>(key_type);
      auto key_as_ref = va_arg(args, Object *);
      key = (key_t)key_as_ref;
      break;
    }
    default: {
      MEMOIR_UNREACHABLE("Attempt to get element with unknown key type");
    }
  }

  // FIXME: equality needs to be updated.
  auto found_key = this->assoc_array.find(key);
  return (found_key != this->assoc_array.end());
}

void AssocArray::remove_element(va_list args) {
  key_t key;

  auto key_type = this->get_key_type();
  switch (key_type->getCode()) {
    case TypeCode::StructTy: {
      key = (key_t)va_arg(args, Object *);
      break;
    }
    case TypeCode::TensorTy: {
      key = (key_t)va_arg(args, Object *);
      break;
    }
    case TypeCode::AssocArrayTy: {
      key = (key_t)va_arg(args, Object *);
      break;
    }
    case TypeCode::SequenceTy: {
      key = (key_t)va_arg(args, Object *);
      break;
    }
    case TypeCode::IntegerTy: {
      auto integer_type = static_cast<IntegerType *>(key_type);
      auto bitwidth = integer_type->bitwidth;
      if (bitwidth < 32) {
        key = (key_t)((integer_type->is_signed) ? va_arg(args, int)
                                                : va_arg(args, int));
      } else if (bitwidth <= 32) {
        key = (key_t)((integer_type->is_signed) ? va_arg(args, int32_t)
                                                : va_arg(args, uint32_t));
      } else if (bitwidth <= 64) {
        key = (key_t)((integer_type->is_signed) ? va_arg(args, int64_t)
                                                : va_arg(args, uint64_t));
      }
      // TODO: do this for all cases.
      this->assoc_array.erase(key);
      return;
    }
    case TypeCode::FloatTy: {
      auto type = static_cast<FloatType *>(key_type);
      key = (key_t)((float)va_arg(args, double));
      break;
    }
    case TypeCode::DoubleTy: {
      auto type = static_cast<DoubleType *>(key_type);
      key = (key_t)va_arg(args, double);
      break;
    }
    case TypeCode::PointerTy: {
      auto type = static_cast<PointerType *>(key_type);
      key = (key_t)va_arg(args, void *);
      break;
    }
    case TypeCode::ReferenceTy: {
      auto type = static_cast<ReferenceType *>(key_type);
      key = (key_t)va_arg(args, Object *);
      break;
    }
    default: {
      MEMOIR_UNREACHABLE("Attempt to get element with unknown key type");
    }
  }

  return;
}

Collection *AssocArray::get_slice(va_list args) {
  MEMOIR_UNREACHABLE("Attempt to get slice of associative array, UNSUPPORTED");

  return nullptr;
}

Collection *AssocArray::join(va_list args, uint8_t num_args) {
  MEMOIR_UNREACHABLE(
      "Attempt to perfrom join operation on associative array, UNSUPPORTED");

  return nullptr;
}

Type *AssocArray::get_element_type() const {
  return this->get_value_type();
}

Type *AssocArray::get_key_type() const {
  auto assoc_array_type = static_cast<AssocArrayType *>(this->get_type());

  return assoc_array_type->key_type;
}

Type *AssocArray::get_value_type() const {
  auto assoc_array_type = static_cast<AssocArrayType *>(this->get_type());

  return assoc_array_type->value_type;
}

uint64_t AssocArray::size() const {
  return this->assoc_array.size();
}

Collection *AssocArray::keys() const {
  std::vector<AssocArray::key_t> assoc_keys;
  assoc_keys.reserve(this->assoc_array.size());
  for (auto const &[k, v] : this->assoc_array) {
    assoc_keys.push_back(k);
  }
  return new SequenceAlloc(SequenceType::get(this->get_key_type()),
                           std::move(assoc_keys));
}

bool AssocArray::equals(const Object *other) const {
  return (this == other);
}

/*
 * Sequence implementation
 */

Sequence::Sequence(SequenceType *type) : Collection(type) {
  // Do nothing.
}

uint64_t Sequence::get_element(va_list args) {
  auto index = va_arg(args, uint64_t);

  return this->get_sequence_element(index);
}

void Sequence::set_element(uint64_t value, va_list args) {
  auto index = va_arg(args, uint64_t);

  return this->set_sequence_element(value, index);
}

bool Sequence::has_element(va_list args) {
  auto index = va_arg(args, uint64_t);

  return this->has_sequence_element(index);
}

void Sequence::remove_element(va_list args) {
  auto index = va_arg(args, uint64_t);

  // TODO
  return;
}

Collection *Sequence::get_slice(va_list args) {
  auto left_index = va_arg(args, int64_t);
  auto right_index = va_arg(args, int64_t);

  return this->get_sequence_slice(left_index, right_index);
}

Collection *Sequence::join(SequenceType *type,
                           std::vector<Sequence *> sequences_to_join) {
  uint64_t initial_size = 0;
  for (auto sequence_to_join : sequences_to_join) {
    initial_size += sequence_to_join->size();
  }

  std::vector<uint64_t> new_vector(sequences_to_join.at(0)->begin(),
                                   sequences_to_join.at(0)->end());
  if (sequences_to_join.size() > 1) {
    for (auto seq_it = sequences_to_join.begin() + 1;
         seq_it != sequences_to_join.end();
         ++seq_it) {
      // TODO: We need to make this an Element::clone, OR change structs to be
      // backed by persistent data structures as well.
      for (auto elem_it = (*seq_it)->begin(); elem_it != (*seq_it)->end();
           ++elem_it) {
        new_vector.push_back(*elem_it);
      }
    }
  }

  auto new_sequence = new SequenceAlloc(type, std::move(new_vector));

  return new_sequence;
}

Collection *Sequence::join(va_list args, uint8_t num_args) {
  MEMOIR_ASSERT((num_args > 0),
                "No arguments given to join operation on a sequence.");

  SequenceType *sequence_type = nullptr;
  std::vector<Sequence *> sequences_to_join = { this };
  sequences_to_join.reserve(num_args);
  for (auto i = 0; i < num_args; i++) {
    auto arg = va_arg(args, Object *);
    MEMOIR_ASSERT((arg != nullptr),
                  "Attempt to perform join operation with NULL sequence.");

    auto arg_type = arg->get_type();
    auto arg_type_code = arg_type->getCode();
    MEMOIR_ASSERT(
        (arg_type_code == TypeCode::SequenceTy),
        "Attempt to perform join operation with non-sequence object.");

    auto arg_as_sequence = static_cast<Sequence *>(arg);
    auto arg_sequence_type = static_cast<SequenceType *>(arg_type);
    if (sequence_type == nullptr) {
      sequence_type = arg_sequence_type;
    }
    MEMOIR_ASSERT(
        (arg_sequence_type->equals(sequence_type)),
        "Attempt to perform join operation with sequences of differing type");

    sequences_to_join.push_back(arg_as_sequence);
  }

  return Sequence::join(sequence_type, sequences_to_join);
}

Type *Sequence::get_element_type() const {
  auto sequence_type = static_cast<SequenceType *>(this->get_type());

  return sequence_type->element_type;
}

/*
 * SequenceAlloc implementation.
 */
SequenceAlloc::SequenceAlloc(SequenceType *type, size_t initial_size)
  : SequenceAlloc(type,
                  std::move(init_elements(type->element_type, initial_size))) {
  // Do nothing.
}

SequenceAlloc::SequenceAlloc(SequenceType *type,
                             std::vector<uint64_t> &&initial)
  : Sequence(type),
    _sequence(initial) {
  // Do nothing.
}

void SequenceAlloc::free() {
  auto *element_type = this->get_element_type();
  if (is_object_type(element_type)) {
    for (auto it = this->_sequence.begin(); it != this->_sequence.end(); ++it) {
      ((Object *)(*it))->free();
    }
  }
}

uint64_t SequenceAlloc::get_sequence_element(uint64_t index) {
  MEMOIR_ASSERT((index < this->size()),
                "Attempt to access out of range element of sequence");

  return this->_sequence.at(index);
}

void SequenceAlloc::set_sequence_element(uint64_t value, uint64_t index) {
  MEMOIR_ASSERT((index < this->size()),
                "Attempt to access out of range element of sequence");

  this->_sequence[index] = value;
}

bool SequenceAlloc::has_sequence_element(uint64_t index) {
  return (index < this->size());
}

Collection *SequenceAlloc::get_sequence_slice(int64_t left_index,
                                              int64_t right_index) {
  /*
   * Check for MAX values in the right and left index.
   *   If right index is NEGATIVE, create slice from left to end of
   * sequence. If left index is NEGATIVE, create slice from end to right
   * of sequence. NOTE: this is a reverse operation.
   */
  if (right_index < 0) {
    right_index = this->size() + right_index + 1;
  }

  if (left_index < 0) {
    left_index = this->size() + left_index + 1;
  }

  /*
   * If right index < left index, create a reverse sequence from right to
   *   left of sequence. NOTE: this may be a reverse operation.
   * Otherwise, get the slice inbetween the left and right index.
   */
  if (left_index > right_index) {
    auto length = left_index - right_index;
    auto type = static_cast<SequenceType *>(this->get_type());

    return new SequenceAlloc(
        type,
        std::move(std::vector<uint64_t>(this->_sequence.begin() + right_index,
                                        this->_sequence.begin() + left_index)));
  } else {
    auto length = right_index - left_index;
    auto type = static_cast<SequenceType *>(this->get_type());

    return new SequenceAlloc(type,
                             std::move(std::vector<uint64_t>(
                                 this->_sequence.begin() + left_index,
                                 this->_sequence.begin() + right_index)));
  }
  return nullptr;
}

uint64_t SequenceAlloc::size() const {
  return this->_sequence.size();
}

Sequence::seq_iter SequenceAlloc::begin() {
  return this->_sequence.begin();
}

Sequence::seq_iter SequenceAlloc::end() {
  return this->_sequence.end();
}

// Mutable operations
void SequenceAlloc::insert(uint64_t index, uint64_t value) {
  this->_sequence.insert(this->_sequence.begin() + index, value);
}

void SequenceAlloc::erase(uint64_t from, uint64_t to) {
  this->_sequence.erase(this->_sequence.begin() + from,
                        this->_sequence.begin() + to);
};

void SequenceAlloc::grow(uint64_t size) {
  this->_sequence.resize(this->_sequence.size() + size);
};

bool SequenceAlloc::equals(const Object *other) const {
  return (this == other);
}

/*
 * SequenceView implementation
 */
SequenceView::SequenceView(SequenceView *seq_view, size_t from, size_t to)
  : SequenceView(seq_view->_sequence,
                 from + seq_view->from,
                 seq_view->from + ((to == -1) ? seq_view->size() : to)) {
  // Do nothing.
}

SequenceView::SequenceView(SequenceAlloc *seq, size_t from, size_t to)
  : _sequence(seq),
    from(from),
    to((to == -1) ? seq->size() : to),
    Sequence(static_cast<SequenceType *>(seq->get_type())) {
  // Do nothing.
}

void SequenceView::free() {
  return;
}

uint64_t SequenceView::get_sequence_element(uint64_t index) {
  MEMOIR_ASSERT((index < this->size()),
                "Attempt to access out of range element of sequence");

  return this->_sequence->get_sequence_element(index + this->from);
}

void SequenceView::set_sequence_element(uint64_t value, uint64_t index) {
  MEMOIR_ASSERT((index < this->size()),
                "Attempt to access out of range element of sequence");

  this->_sequence->set_sequence_element(value, index + this->from);
}

bool SequenceView::has_sequence_element(uint64_t index) {
  return (index < this->size());
}

Collection *SequenceView::get_sequence_slice(int64_t left_index,
                                             int64_t right_index) {

  return this->_sequence->get_sequence_slice(left_index + this->from,
                                             right_index + this->to);
}

uint64_t SequenceView::size() const {
  return this->to - this->from;
}

Sequence::seq_iter SequenceView::begin() {
  return this->begin() + this->from;
}

Sequence::seq_iter SequenceView::end() {
  return this->begin() + this->to;
}

bool SequenceView::equals(const Object *other) const {
  return (this == other);
}

// Mutable operations
void SequenceView::insert(uint64_t index, uint64_t value) {
  this->_sequence->insert(index + this->from, value);
}

void SequenceView::erase(uint64_t from, uint64_t to) {
  this->_sequence->erase(from + this->from, to + this->from);
};

void SequenceView::grow(uint64_t size) {
  this->_sequence->grow(size);
};

} // namespace memoir
