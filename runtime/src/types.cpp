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
Type::Type(TypeCode code) : code(code) {
  // Do nothing
}

Type::~Type() {
  // Do nothing
}

/*
 * Object Type
 */
ObjectType::ObjectType() : Type(TypeCode::ObjectTy) {
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
ArrayType::ArrayType(Type *type)
  : Type(TypeCode::ArrayTy),
    elementType(type) {
  // Do nothing.
}

ArrayType::~ArrayType() {
  // Do nothing.
}

/*
 * Union Type
 */
UnionType::UnionType() : Type(TypeCode::UnionTy) {
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

} // namespace objectir
