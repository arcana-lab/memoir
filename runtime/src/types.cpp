#include <iostream>

#include "types.h"

namespace memoir {

TypeCode Type::getCode() {
  return this->code;
}

/*
 * Helper functions
 */
bool isObjectType(Type *type) {
  TypeCode code = type->getCode();
  switch (code) {
    case StructTy:
    case TensorTy:
      return true;
    default:
      return false;
  }
}

bool isIntrinsicType(Type *type) {
  TypeCode code = type->getCode();
  switch (code) {
    case IntegerTy:
    case FloatTy:
    case DoubleTy:
    case ReferenceTy:
      return true;
    default:
      return false;
  }
}

/*
 * Type base class
 */
Type::Type(TypeCode code, const char *name) : code(code) {
  // Do nothing
}

Type::Type(TypeCode code) : Type::Type(code, "") {
  // Do nothing
}

/*
 * Struct Type
 */
std::unordered_map<const char *, Type *> &StructType::struct_types() {
  static std::unordered_map<const char *, Type *> struct_types;

  return struct_types;
}

Type *StructType::get(const char *name) {
  auto found_type = StructType::struct_types().find(name);
  if (found_type != StructType::struct_types().end()) {
    return found_type->second;
  }

  /*
   * Create an empty struct type that will be resolved later.
   */
  std::vector<Type *> empty_fields;
  empty_fields.clear();
  auto empty_struct = new StructType(name, empty_fields);

  StructType::struct_types()[name] = empty_struct;

  return empty_struct;
}

Type *StructType::define(const char *name, std::vector<Type *> &field_types) {
  auto struct_type = (StructType *)StructType::get(name);

  /*
   * Update the struct type entry with the field types
   */
  struct_type->fields = field_types;

  return struct_type;
}

StructType::StructType(const char *name, std::vector<Type *> &field_types)
  : Type(TypeCode::StructTy, name),
    fields(field_types) {
  // Do nothing.
}

bool StructType::equals(Type *other) {
  return (this == other);
}

/*
 * Tensor Type
 */
const uint64_t TensorType::unknown_length = 0;

Type *TensorType::get(Type *element_type, uint64_t num_dimensions) {
  std::vector<uint64_t> length_of_dimensions;

  for (auto i = 0; i < num_dimensions; i++) {
    length_of_dimensions.push_back(0);
  }

  return new TensorType(element_type, num_dimensions, length_of_dimensions);
}

Type *TensorType::get(Type *element_type,
                      uint64_t num_dimensions,
                      std::vector<uint64_t> &length_of_dimensions) {
  return new TensorType(element_type, num_dimensions, length_of_dimensions);
}

TensorType::TensorType(Type *element_type, uint64_t num_dimensions)
  : Type(TypeCode::TensorTy),
    element_type(element_type),
    num_dimensions(num_dimensions),
    is_static_length(false) {
  /*
   * Build the length of dimensions with all unknowns.
   */
  for (auto i = 0; i < num_dimensions; i++) {
    this->length_of_dimensions.push_back(TensorType::unknown_length);
  }
}

TensorType::TensorType(Type *element_type,
                       uint64_t num_dimensions,
                       std::vector<uint64_t> &length_of_dimensions)
  : Type(TypeCode::TensorTy),
    element_type(element_type),
    num_dimensions(num_dimensions),
    length_of_dimensions(length_of_dimensions) {
  /*
   * Determine if all dimensions of the tensor are of static length.
   */
  this->is_static_length = true;
  for (auto length : length_of_dimensions) {
    if (length == TensorType::unknown_length) {
      this->is_static_length = false;
      break;
    }
  }
}

bool TensorType::equals(Type *other) {
  if (this->getCode() != other->getCode()) {
    return false;
  }

  auto other_as_tensor = (TensorType *)other;
  auto other_num_dimensions = other_as_tensor->num_dimensions;
  if (this->num_dimensions != other_num_dimensions) {
    return false;
  }

  for (auto i = 0; i < this->num_dimensions; i++) {
    auto this_length = this->length_of_dimensions.at(i);
    auto other_length = other_as_tensor->length_of_dimensions.at(i);
    if (this_length != other_length) {
      return false;
    }
  }

  auto other_element_type = other_as_tensor->element_type;
  return this->element_type->equals(other_element_type);
}

/*
 * Integer Type
 */
Type *IntegerType::get(unsigned bitwidth, bool is_signed) {
  static std::unordered_map<
      // bitwidth
      unsigned,
      std::unordered_map<
          // is_signed
          bool,
          IntegerType *>>
      integer_types;

  auto found_bitwidth = integer_types.find(bitwidth);
  if (found_bitwidth != integer_types.end()) {
    auto bitwidth_types = found_bitwidth->second;
    auto found_signed = bitwidth_types.find(is_signed);
    if (found_signed != bitwidth_types.end()) {
      auto integer_type = found_signed->second;
      return integer_type;
    }
  }

  auto new_integer_type = new IntegerType(bitwidth, is_signed);
  integer_types[bitwidth][is_signed] = new_integer_type;
  return new_integer_type;
}

IntegerType::IntegerType(unsigned bitwidth, bool is_signed)
  : Type(TypeCode::IntegerTy),
    bitwidth(bitwidth),
    is_signed(is_signed) {
  // Do nothing.
}

bool IntegerType::equals(Type *other) {
  if (this == other) {
    return true;
  }

  if (this->getCode() != other->getCode()) {
    return false;
  }

  auto other_as_int = (IntegerType *)other;
  if (this->bitwidth != other_as_int->bitwidth) {
    return false;
  }

  if (this->is_signed != other_as_int->is_signed) {
    return false;
  }

  return true;
}

/*
 * Float Type
 */
Type *FloatType::get() {
  static FloatType float_type;

  return &float_type;
}

FloatType::FloatType() : Type(TypeCode::FloatTy) {
  // Do nothing.
}

bool FloatType::equals(Type *other) {
  return (this->getCode() == other->getCode());
}

/*
 * Double Type
 */
Type *DoubleType::get() {
  static DoubleType double_type;

  return &double_type;
}

DoubleType::DoubleType() : Type(TypeCode::DoubleTy) {
  // Do nothing.
}

bool DoubleType::equals(Type *other) {
  return (this->getCode() == other->getCode());
}

/*
 * Reference Type
 */
Type *ReferenceType::get(Type *referenced_type) {
  return new ReferenceType(referenced_type);
}

ReferenceType::ReferenceType(Type *referenced_type)
  : Type(TypeCode::ReferenceTy),
    referenced_type(referenced_type) {
  if (!isObjectType(referenced_type)) {
    std::cerr << "ERROR: Contained type of reference is not an object!\n";
    exit(1);
  }
}

bool ReferenceType::equals(Type *other) {
  if (this->getCode() != other->getCode()) {
    return false;
  }

  auto other_ref = (ReferenceType *)other;
  auto this_referenced_type = this->referenced_type;
  auto other_referenced_type = other_ref->referenced_type;
  return this_referenced_type->equals(other_referenced_type);
}

} // namespace memoir
