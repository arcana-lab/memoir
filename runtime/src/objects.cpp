#include <iostream>

#include "internal.h"
#include "objects.h"

namespace memoir {

/*
 * Abstract Object implementation
 */

Type *Object::get_type() const {
  return this->type;
}

Object::Object(Type *type) : type(type) {
  // Do nothing.
}

bool Object::is_collection() const {
  return false;
}

bool Object::is_struct() const {
  return false;
}

bool Object::is_element() const {
  return false;
}

/*
 * Struct Objects
 */
Struct::Struct(Type *type) : Object(type) {
  MEMOIR_ASSERT((type->getCode() == TypeCode::StructTy),
                "Trying to create a struct of non-struct type");

  // Initialize the fields
  auto object_type = (StructType *)(type);
  for (auto field_type : object_type->fields) {
    auto field = Element::create(field_type);
    this->fields.push_back(field);
  }
}

Element *Struct::get_field(uint64_t field_index) const {
  MEMOIR_ASSERT((field_index < this->fields.size()),
                "Trying to read field from index outside of struct's range");

  return this->fields.at(field_index);
}

bool Struct::equals(const Object *other) const {
  return (this == other);
}

bool Struct::is_struct() const {
  return true;
}

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

  // Initialize the tensor
  for (auto dimension_length : this->length_of_dimensions) {
    for (auto i = 0; i < dimension_length; i++) {
      auto field = Element::create(element_type);
      this->tensor.push_back(field);
    }
  }
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

  for (auto i = 0; i < flattened_length; i++) {
    auto field = Element::create(element_type);
    this->tensor.push_back(field);
  }
}

Element *Tensor::get_tensor_element(std::vector<uint64_t> &indices) const {
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

Element *Tensor::get_element(va_list args) {
  auto num_dimensions = this->length_of_dimensions.size();

  std::vector<uint64_t> indices = {};
  for (auto i = 0; i < num_dimensions; i++) {
    auto index = va_arg(args, uint64_t);
    indices.push_back(index);
  }

  return this->get_tensor_element(indices);
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

AssocArray::key_value_pair_t &AssocArray::get_pair(Object *key) {
  // Find the key if it already exists
  for (auto it = this->assoc_array.begin(); it != this->assoc_array.end();
       ++it) {
    auto &pair = *it;
    if (key->equals(pair.first)) {
      return pair;
    }
  }

  // Create a new key-value pair and return it.
  this->assoc_array[key] = Element::create(this->get_value_type());

  return *(this->assoc_array.find(key));
}

Element *AssocArray::get_element(va_list args) {
  Object *key;

  auto key_type = this->get_key_type();
  switch (key_type->getCode()) {
    case TypeCode::StructTy: {
      key = va_arg(args, Object *);
      break;
    }
    case TypeCode::TensorTy: {
      key = va_arg(args, Object *);
      break;
    }
    case TypeCode::AssocArrayTy: {
      key = va_arg(args, Object *);
      break;
    }
    case TypeCode::SequenceTy: {
      key = va_arg(args, Object *);
      break;
    }
    case TypeCode::IntegerTy: {
      auto integer_type = static_cast<IntegerType *>(key_type);
      uint64_t key_as_int = 0;
      switch (integer_type->bitwidth) {
        case 64: {
          key_as_int =
              (uint64_t)((integer_type->is_signed) ? va_arg(args, int64_t)
                                                   : va_arg(args, uint64_t));
          break;
        }
        case 32: {
          key_as_int =
              (uint64_t)((integer_type->is_signed) ? va_arg(args, int32_t)
                                                   : va_arg(args, uint32_t));
          break;
        }
        case 16: {
          key_as_int =
              (uint64_t)((integer_type->is_signed) ? va_arg(args, int)
                                                   : va_arg(args, int));
          break;
        }
        case 8: {
          key_as_int =
              (uint64_t)((integer_type->is_signed) ? va_arg(args, int)
                                                   : va_arg(args, int));
          break;
        }
        case 1: {
          key_as_int =
              (uint64_t)((integer_type->is_signed) ? va_arg(args, int)
                                                   : va_arg(args, int));
          break;
        }
      }
      key = new IntegerElement(integer_type, key_as_int);
      break;
    }
    case TypeCode::FloatTy: {
      auto type = static_cast<FloatType *>(key_type);
      auto key_as_float = (float)va_arg(args, double);
      key = new FloatElement(type, key_as_float);
      break;
    }
    case TypeCode::DoubleTy: {
      auto type = static_cast<DoubleType *>(key_type);
      auto key_as_double = va_arg(args, double);
      key = new DoubleElement(type, key_as_double);
      break;
    }
    case TypeCode::PointerTy: {
      auto type = static_cast<PointerType *>(key_type);
      auto key_as_ptr = va_arg(args, void *);
      key = new PointerElement(type, key_as_ptr);
      break;
    }
    case TypeCode::ReferenceTy: {
      auto type = static_cast<ReferenceType *>(key_type);
      auto key_as_ref = va_arg(args, Object *);
      key = new ReferenceElement(type, key_as_ref);
      break;
    }
    default: {
      MEMOIR_UNREACHABLE("Attempt to get element with unknown key type");
    }
  }

  return this->get_pair(key).second;
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

bool AssocArray::equals(const Object *other) const {
  return (this == other);
}

/*
 * Sequence implementation
 */
Sequence::Sequence(Type *type, uint64_t initial_size) : Collection(type) {
  MEMOIR_ASSERT((type->getCode() == TypeCode::SequenceTy),
                "Attempt to create sequence of non-sequence type");

  for (auto i = 0; i < initial_size; i++) {
    this->sequence.push_back(Element::create(this->get_element_type()));
  }
}

Element *Sequence::get_element(uint64_t index) {
  MEMOIR_ASSERT((index < this->sequence.size()),
                "Attempt to access out of range element of sequence");

  return this->sequence.at(index);
}

Element *Sequence::get_element(va_list args) {
  auto index = va_arg(args, uint64_t);

  return this->get_element(index);
}

Collection *Sequence::get_slice(int64_t left_index, int64_t right_index) {
  /*
   * Check for MAX values in the right and left index.
   *   If right index is NEGATIVE, create slice from left to end of
   * sequence. If left index is NEGATIVE, create slice from end to right
   * of sequence. NOTE: this is a reverse operation.
   */
  if (right_index < 0) {
    right_index = this->sequence.size() + right_index;
  }

  if (left_index < 0) {
    left_index = this->sequence.size() + left_index;
  }

  std::cerr << "Slicing sequence of size "
            << std::to_string(this->sequence.size()) << " from "
            << std::to_string(left_index) << " : "
            << std::to_string(right_index) << "\n";

  /*
   * If right index < left index, create a reverse sequence from right to
   *   left of sequence. NOTE: this may be a reverse operation.
   * Otherwise, get the slice inbetween the left and right index.
   */
  if (left_index > right_index) {
    auto length = left_index - right_index + 1;
    auto type = static_cast<SequenceType *>(this->get_type());
    auto slice = new Sequence(type, length);
    for (auto i = 0; i < length; i++) {
      slice->sequence[i] = this->get_element(left_index - i)->clone();
    }
    return slice;
  } else {
    auto length = right_index - left_index + 1;
    auto type = static_cast<SequenceType *>(this->get_type());
    auto slice = new Sequence(type, length);
    for (auto i = 0; i < length; i++) {
      slice->sequence[i] = this->get_element(left_index + i)->clone();
    }
    return slice;
  }
}

Collection *Sequence::get_slice(va_list args) {
  auto left_index = va_arg(args, int64_t);
  auto right_index = va_arg(args, int64_t);

  return this->get_slice(left_index, right_index);
}

Collection *Sequence::join(SequenceType *type,
                           std::vector<Sequence *> sequences_to_join) {
  auto new_sequence = new Sequence(type, 0);

  for (auto sequence_to_join : sequences_to_join) {
    for (auto element : sequence_to_join->sequence) {
      new_sequence->sequence.push_back(element->clone());
    }
  }

  return new_sequence;
}

Collection *Sequence::join(va_list args, uint8_t num_args) {
  MEMOIR_ASSERT((num_args > 0),
                "No arguments given to join operation on a sequence.");

  SequenceType *sequence_type = nullptr;
  std::vector<Sequence *> sequences_to_join = { this };
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

bool Sequence::equals(const Object *other) const {
  return (this == other);
}

} // namespace memoir
