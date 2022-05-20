#include <iostream>
#include <stdarg.h>

#include "object_ir.h"

using namespace objectir;

extern "C" {

/*
 * Type construction
 */
__OBJECTIR_ATTR
Type *getObjectType(int numFields, ...) {
  auto type = new ObjectType();

  va_list args;

  va_start(args, numFields);

  for (int i = 0; i < numFields; i++) {
    auto arg = va_arg(args, Type *);
    type->fields.push_back(arg);
  }

  va_end(args);

  return type;
}

__OBJECTIR_ATTR
Type *nameObjectType(char *name, int numFields, ...) {
  auto type = new ObjectType(name);

  auto typeFactory = TypeFactory::getInstance();
  typeFactory->registerType(name, type);

  va_list args;

  va_start(args, numFields);

  for (int i = 0; i < numFields; i++) {
    auto arg = va_arg(args, Type *);
    type->fields.push_back(arg);
  }

  va_end(args);

  return type;
}

__OBJECTIR_ATTR
Type *getArrayType(Type *type) {
  return new ArrayType(type);
}

__OBJECTIR_ATTR
Type *nameArrayType(char *name, Type *elementType) {
  auto type = new ArrayType(elementType, name);

  auto typeFactory = TypeFactory::getInstance();
  typeFactory->registerType(name, type);

  return type;
}

__OBJECTIR_ATTR
Type *getUnionType(int numMembers, ...) {
  auto type = new UnionType();

  va_list args;

  va_start(args, numMembers);

  for (int i = 0; i < numMembers; i++) {
    auto arg = va_arg(args, Type *);
    type->members.push_back(arg);
  }

  va_end(args);

  return type;
}

__OBJECTIR_ATTR
Type *nameUnionType(char *name, int numMembers, ...) {
  auto type = new UnionType(name);

  auto typeFactory = TypeFactory::getInstance();
  typeFactory->registerType(name, type);

  va_list args;

  va_start(args, numMembers);

  for (int i = 0; i < numMembers; i++) {
    auto arg = va_arg(args, Type *);
    type->members.push_back(arg);
  }

  va_end(args);

  return type;
}

__OBJECTIR_ATTR
Type *getNamedType(char *name) {
  auto typeFactory = TypeFactory::getInstance();
  return typeFactory->getType(name);
}

__OBJECTIR_ATTR
Type *getIntegerType(uint64_t bitwidth, bool isSigned) {
  return new IntegerType(bitwidth, isSigned);
}

__OBJECTIR_ATTR
Type *getUInt64Type() {
  return new IntegerType(64, false);
}

__OBJECTIR_ATTR
Type *getUInt32Type() {
  return new IntegerType(32, false);
}

__OBJECTIR_ATTR
Type *getUInt16Type() {
  return new IntegerType(16, false);
}

__OBJECTIR_ATTR
Type *getUInt8Type() {
  return new IntegerType(8, false);
}

__OBJECTIR_ATTR
Type *getInt64Type() {
  return new IntegerType(64, true);
}

__OBJECTIR_ATTR
Type *getInt32Type() {
  return new IntegerType(32, true);
}

__OBJECTIR_ATTR
Type *getInt16Type() {
  return new IntegerType(16, true);
}

__OBJECTIR_ATTR
Type *getInt8Type() {
  return new IntegerType(8, true);
}

__OBJECTIR_ATTR
Type *getBooleanType() {
  return new IntegerType(1, false);
}

__OBJECTIR_ATTR
Type *getFloatType() {
  return new FloatType();
}

__OBJECTIR_ATTR
Type *getDoubleType() {
  return new DoubleType();
}

__OBJECTIR_ATTR
Type *getPointerType(Type *containedType) {
  return new PointerType(containedType);
}

/*
 * Object construction
 */
__ALLOC_ATTR
__OBJECTIR_ATTR
Object *buildObject(Type *type) {
  auto obj = new Object(type);

  return obj;
}

__ALLOC_ATTR
__OBJECTIR_ATTR
Array *buildArray(Type *type, uint64_t length) {
  auto array = new Array(type, length);

  return array;
}

__OBJECTIR_ATTR
Union *buildUnion(Type *type) {
  auto unionObj = new Union(type);

  return unionObj;
}

__OBJECTIR_ATTR
void deleteObject(Object *obj) {
  delete obj;
}

} // extern "C"
