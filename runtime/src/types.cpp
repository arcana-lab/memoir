#include "types.hpp"

using namespace objectir;

Type::Type()
  : isObject(false),
    isArray(false),
    isInteger(false),
    isFloat(false),
    isDouble(false) {
  // Do nothing.
}

ObjectType::ObjectType() : Type() {
  this->isObject = true;
}

ObjectType::~ObjectType() {
  for (auto field : this->fields) {
    delete field;
  }
}

ArrayType::ArrayType(Type *type)
  : Type(),
    elementType(type) {
  this->isArray = true;
}

IntegerType::IntegerType(uint64_t bitwidth, bool isSigned)
  : Type(),
    bitwidth(bitwidth),
    isSigned(isSigned) {
  this->isInteger = true;
}

FloatType::FloatType() : Type() {
  this->isFloat = true;
}

DoubleType::DoubleType() : Type() {
  this->isDouble = true;
}
