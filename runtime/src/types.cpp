#include "types.hpp"

using namespace objectir;

/*
 * Type base class
 */
Type::Type()
  : isObject(false),
    isArray(false),
    isInteger(false),
    isFloat(false),
    isDouble(false) {
  // Do nothing.
}

/*
 * Object Type
 */
ObjectType::ObjectType() : Type() {
  this->isObject = true;
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
  : Type(),
    elementType(type) {
  this->isArray = true;
}

ArrayType::~ArrayType() {
  delete elementType;
}

/*
 * Union Type
 */
UnionType::UnionType() : Type() {
  this->isUnion = true;
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
  : Type(),
    bitwidth(bitwidth),
    isSigned(isSigned) {
  this->isInteger = true;
}

/*
 * Float Type
 */
FloatType::FloatType() : Type() {
  this->isFloat = true;
}

/*
 * Double Type
 */
DoubleType::DoubleType() : Type() {
  this->isDouble = true;
}
