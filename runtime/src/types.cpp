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
    case StubTy:
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

bool isStubType(Type *type) {
  TypeCode code = type->getCode();
  switch (code) {
    case StubTy:
      return true;
    default:
      return false;
  }
}

/*
 * Type base class
 */
Type *Type::find(std::string name) {
  auto found_named = named_types.find(name);
  if (found_named != named_types.end()) {
    return found_named->second;
  }

  return nullptr;
}

void Type::define(std::string name, Type *type_to_define) {
  named_types[name] = type_to_define;
}

Type::Type(TypeCode code, std::string name) : code(code) {
  // Do nothing
}

Type::Type(TypeCode code) : Type::Type(code, "") {
  // Do nothing
}

/*
 * Struct Type
 */
Type *StructType::get(std::string name, std::vector<Type *> &field_types) {
  auto found_type = Type::find(name);
  if (found_type) {
    return found_type;
  }

  auto new_struct = new StructType(name, field_types);

  Type::define(name, new_struct);

  return new_struct;
}

StructType::StructType(std::string name, std::vector<Type *> &field_types)
  : Type(TypeCode::StructTy, name),
    fields(field_types) {
  // Do nothing.
}

Type *StructType::resolve() {
  for (auto field_iter = this->fields.begin(); field_iter != this->fields.end();
       ++field_iter) {
    *field_iter = (*field_iter)->resolve();
  }

  return this;
}

bool StructType::equals(Type *other) {
  return (this == other);
}

/*
 * Tensor Type
 */
Type *TensorType::get(Type *element_type, uint64_t num_dimensions) {
  return new TensorType(element_type, num_dimensions);
}

TensorType::TensorType(Type *type, uint64_t num_dimensions)
  : Type(TypeCode::TensorTy),
    element_type(type),
    num_dimensions(num_dimensions) {
  // Do nothing.
}

Type *TensorType::resolve() {
  this->element_type = this->element_type->resolve();
  return this;
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

  auto other_element_type = other_as_tensor->element_type;
  return this->element_type->equals(other_element_type);
}

/*
 * Integer Type
 */
Type *IntegerType::get(unsigned bitwidth, bool is_signed) {
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

Type *IntegerType::resolve() {
  return this;
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

Type *FloatType::resolve() {
  return this;
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

Type *DoubleType::resolve() {
  return this;
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

Type *ReferenceType::resolve() {
  this->referenced_type = this->referenced_type->resolve();
  return this;
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

/*
 * Stub Type
 */
Type *StubType::get(std::string name) {
  auto found_resolved = Type::find(name);
  if (found_resolved) {
    return found_resolved;
  }

  auto found_stub = StubType::stub_types.find(name);
  if (found_stub != StubType::stub_types.end()) {
    return found_stub->second;
  }

  auto new_stub = new StubType(name);

  StubType::stub_types[name] = new_stub;

  return new_stub;
}

StubType::StubType(std::string name) : Type(TypeCode::StubTy), name(name) {
  // Do nothing.
}

Type *StubType::resolve() {
  auto found_resolved = Type::find(this->name);
  if (found_resolved) {
    this->resolved_type = found_resolved;
    return found_resolved;
  }

  // Unable to resolve the stub type, error.
  if (!this->resolved_type) {
    std::cerr << "ERROR: " << name
              << " is not resolved to a type before it's use.";
    exit(1);
  }

  return this->resolved_type;
}

bool StubType::equals(Type *other) {
  auto otherStub = (StubType *)other;
  return (this->name == otherStub->name);
}

} // namespace memoir
