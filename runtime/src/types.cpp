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

Type *ObjectType::resolve() {
  if (resolved) {
    return this;
  }
  resolved = true;
  for (auto it = this->fields.begin();
       it != this->fields.end();
       ++it) {
    *it = (*it)->resolve();
  }

  return this;
}

bool ObjectType::equals(Type *other) {
  return (this == other);
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

Type *ArrayType::resolve() {
  if (resolved) {
    return this;
  }
  resolved = true;
  this->elementType = this->elementType->resolve();
  return this;
}

bool ArrayType::equals(Type *other) {
  return (this == other);
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

Type *UnionType::resolve() {
  // TODO: resolve members
  if (resolved) {
    return this;
  }
  resolved = true;

  return this;
}

bool UnionType::equals(Type *other) {
  return (this == other);
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

Type *IntegerType::resolve() {
  if (resolved) {
    return this;
  }
  resolved = true;
  return this;
}

bool IntegerType::equals(Type *other) {
  if (this->getCode() != other->getCode()) {
    return false;
  }

  auto otherInt = (IntegerType *)other;
  if (this->bitwidth != otherInt->bitwidth) {
    return false;
  }

  if (this->isSigned != otherInt->isSigned) {
    return false;
  }

  return true;
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

Type *FloatType::resolve() {
  if (resolved) {
    return this;
  }
  resolved = true;
  return this;
}

bool FloatType::equals(Type *other) {
  return (this->getCode() == other->getCode());
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

Type *DoubleType::resolve() {
  if (resolved) {
    return this;
  }
  resolved = true;

  return this;
}

bool DoubleType::equals(Type *other) {
  return (this->getCode() == other->getCode());
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

Type *PointerType::resolve() {
  if (resolved) {
    return this;
  }
  resolved = true;

  this->containedType = this->containedType->resolve();
  return this;
}

bool PointerType::equals(Type *other) {
  if (this->getCode() != other->getCode()) {
    return false;
  }

  auto otherPtr = (PointerType *)other;
  auto thisContained = this->containedType;
  auto otherContained = otherPtr->containedType;
  return thisContained->equals(otherContained);
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
  if (resolved) {
    return this;
  }
  resolved = true;

  // Resolve the stub type.
  if (!this->resolvedType) {
    std::cerr
        << "ERROR: " << name
        << " is not resolved to a type before it's use.";
    exit(1);
  }

  return this->resolvedType;
}

bool StubType::equals(Type *other) {
  auto otherStub = (StubType *)other;
  return (this->name == otherStub->name);
}

} // namespace objectir
