#include <iostream>

#include "internal.h"
#include "types.h"

namespace memoir {

TypeCode Type::getCode() {
  return this->code;
}

/*
 * Helper functions
 */
bool is_object_type(Type *type) {
  TypeCode code = type->getCode();
  switch (code) {
    case StructTy:
    case AssocArrayTy:
    case SequenceTy:
      return true;
    default:
      return false;
  }
}

bool is_struct_type(Type *type) {
  TypeCode code = type->getCode();
  switch (code) {
    case StructTy:
      return true;
    default:
      return false;
  }
}

bool is_collection_type(Type *type) {
  TypeCode code = type->getCode();
  switch (code) {
    case AssocArrayTy:
    case SequenceTy:
      return true;
    default:
      return false;
  }
}

bool is_intrinsic_type(Type *type) {
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
Type::Type(TypeCode code, const char *name) : code(code), name(name) {
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

StructType *StructType::get(const char *name) {
  auto found_type = StructType::struct_types().find(name);
  if (found_type != StructType::struct_types().end()) {
    return static_cast<StructType *>(found_type->second);
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

StructType *StructType::define(const char *name,
                               std::vector<Type *> &field_types) {
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
 * Associative Array Type
 */
AssocArrayType *AssocArrayType::get(Type *key_type, Type *value_type) {
  // TODO, make these unique
  return new AssocArrayType(key_type, value_type);
}

AssocArrayType::AssocArrayType(Type *key_type, Type *value_type)
  : Type(TypeCode::AssocArrayTy),
    key_type(key_type),
    value_type(value_type) {
  // Do nothing.
}

bool AssocArrayType::equals(Type *other) {
  if (this->getCode() != other->getCode()) {
    return false;
  }

  auto other_type = static_cast<AssocArrayType *>(other);
  return ((this->key_type == other_type->key_type)
          && (this->value_type == other_type->value_type));
}

/*
 * Sequence Type
 */

std::unordered_map<Type *, SequenceType *> &SequenceType::sequence_types() {
  static std::unordered_map<Type *, SequenceType *> sequence_types;

  return sequence_types;
}

SequenceType *SequenceType::get(Type *element_type) {
  // See if we already have a sequence type for this element type.
  auto &sequence_types = SequenceType::sequence_types();
  auto found_type = sequence_types.find(element_type);
  if (found_type != sequence_types.end()) {
    return static_cast<SequenceType *>(found_type->second);
  }

  // If we couldn't find a sequence type for this element type, create and
  // memoize a new one.
  auto new_type = new SequenceType(element_type);
  sequence_types[element_type] = new_type;
  return new_type;
}

SequenceType::SequenceType(Type *element_type)
  : Type(TypeCode::SequenceTy),
    element_type(element_type) {
  // Do nothing.
}

bool SequenceType::equals(Type *other) {
  if (this->getCode() != other->getCode()) {
    return false;
  }

  auto other_type = static_cast<SequenceType *>(other);
  return (this->element_type == other_type->element_type);
}

/*
 * Integer Type
 */
IntegerType *IntegerType::get(unsigned bitwidth, bool is_signed) {
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
FloatType *FloatType::get() {
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
DoubleType *DoubleType::get() {
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
 * Pointer Type
 */
PointerType *PointerType::get() {
  static PointerType pointer_type;

  return &pointer_type;
}

PointerType::PointerType() : Type(TypeCode::PointerTy) {
  // Do nothing
}

bool PointerType::equals(Type *other) {
  return (this->getCode() == other->getCode());
}

// Void Type
VoidType *VoidType::get() {
  static VoidType void_type;

  return &void_type;
}

VoidType::VoidType() : Type(TypeCode::VoidTy) {
  // Do nothing.
}

bool VoidType::equals(Type *other) {
  return (this->getCode() == other->getCode());
}

/*
 * Reference Type
 */
ReferenceType *ReferenceType::get(Type *referenced_type) {
  return new ReferenceType(referenced_type);
}

ReferenceType::ReferenceType(Type *referenced_type)
  : Type(TypeCode::ReferenceTy),
    referenced_type(referenced_type) {
  MEMOIR_ASSERT(is_object_type(referenced_type),
                "Attempt to define reference type to non-memoir Object.");
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
