#include "types.h"

using namespace objectir;

/*
 * Type base class
 */
Type::Type(TypeCode code) {
  // Do nothing.
}

/*
 * Object Type
 */
ObjectType::ObjectType() : Type(TypeCode::ObjectTy) {
  // Do nothing.
}

ObjectType::~ObjectType() {
  for (auto field : this->fields) {
    delete field;
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
  delete elementType;
}

/*
 * Union Type
 */
UnionType::UnionType() : Type(TypeCode::UnionTy) {
  // Do nothing.
}

UnionType::~UnionType() {
  for (auto member : this->members) {
    delete member;
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
