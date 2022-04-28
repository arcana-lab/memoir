#include <iostream>

#include "types.h"

namespace objectir {

TypeCode Type::getCode() {
  return this->code;
}

/*
 * Helper functions
 */
bool isObjectType(Type *type) {
  TypeCode code = type->getCode();
  switch (code) {
    case ObjectTy:
    case ArrayTy:
    case UnionTy:
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
      return true;
    default:
      return false;
  }
}

/*
 * Type base class
 */
Type::Type(TypeCode code, std::string name) : code(code) {
  // Do nothing
}

Type::Type(TypeCode code) : Type::Type(code, "") {
  // Do nothing
}

Type::~Type() {
  // Do nothing
}

/*
 * Object Type
 */

ObjectType::ObjectType(std::string name)
  : Type(TypeCode::ObjectTy, name) {
  // Do nothing.
}

ObjectType::ObjectType() : ObjectType::ObjectType("") {
  // Do nothing.
}

ObjectType::~ObjectType() {
  for (auto field : this->fields) {
    // delete field;
  }
}

/*
 * Array Type
 */
ArrayType::ArrayType(Type *type, std::string name)
  : Type(TypeCode::ArrayTy, name),
    elementType(type) {
  // Do nothing.
}

ArrayType::ArrayType(Type *type)
  : ArrayType::ArrayType(type, "") {
  // Do nothing.
}

ArrayType::~ArrayType() {
  // Do nothing.
}

/*
 * Union Type
 */
UnionType::UnionType(std::string name)
  : Type(TypeCode::UnionTy, name) {
  // Do nothing.
}

UnionType::UnionType() : UnionType::UnionType("") {
  // Do nothing.
}

UnionType::~UnionType() {
  for (auto member : this->members) {
    // delete member;
  }
}

/*
 * Integer Type
 */
IntegerType::IntegerType(uint64_t bitwidth, bool isSigned)
  : Type(TypeCode::IntegerTy),
    bitwidth(bitwidth),
    isSigned(isSigned) {
  // Do nothing.
}
IntegerType::~IntegerType() {
  // Do nothing
}

/*
 * Float Type
 */
FloatType::FloatType() : Type(TypeCode::FloatTy) {
  // Do nothing.
}

FloatType::~FloatType() {
  // Do nothing;
}

/*
 * Double Type
 */
DoubleType::DoubleType() : Type(TypeCode::DoubleTy) {
  // Do nothing.
}

DoubleType::~DoubleType() {
  // Do nothing.
}

/*
 * Pointer Type
 */
PointerType::PointerType(Type *containedType)
  : Type(TypeCode::PointerTy),
    containedType(containedType) {
  if (!isObjectType(containedType)) {
    std::cerr
        << "ERROR: Contained type of pointer is not an object!\n";
    exit(1);
  }
}

PointerType::~PointerType() {
  // Do nothing.
}

/*
 * Stub Type
 */
StubType::StubType(std::string name)
  : Type(TypeCode::StubTy),
    name(name) {
  // Do nothing.
}

Type *StubType::resolve() {
  // Resolve the stub type.
  if (!this->resolvedType) {
    std::cerr
        << "ERROR: " << name
        << " is not resolved to a type before it's use.";
    exit(1);
  }

  return this->resolvedType;
}

} // namespace objectir
