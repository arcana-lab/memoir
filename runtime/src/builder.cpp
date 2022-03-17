#include "builder.hpp"

using namespace objectir;

extern "C" {

/*
 * Type construction
 */
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

Type *getArrayType(Type *type) {
  return new ArrayType(type);
}

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

Type *getIntegerType(uint64_t bitwidth,
                               bool isSigned) {
  return new IntegerType(bitwidth, isSigned);
}

Type *getUInt64Type() {
  return new IntegerType(64, false);
}

Type *getUInt32Type() {
  return new IntegerType(32, false);
}

Type *getUInt16Type() {
  return new IntegerType(16, false);
}

Type *getUInt8Type() {
  return new IntegerType(8, false);
}

Type *getInt64Type() {
  return new IntegerType(64, true);
}

Type *getInt32Type() {
  return new IntegerType(32, true);
}

Type *getInt16Type() {
  return new IntegerType(16, true);
}

Type *getInt8Type() {
  return new IntegerType(8, true);
}

Type *getBooleanType() {
  return new IntegerType(1, false);
}

Type *getFloatType() {
  return new FloatType();
}

Type *getDoubleType() {
  return new DoubleType();
}

/*
 * Object construction
 */
Object *ObjectBuilder::buildObject(Type *type) {
  auto obj = new Object(type);

  return obj;
}

Object *ObjectBuilder::buildArray(Type *type, uint64_t length) {
  auto array = new Array(type, length);

  return array;
}

Object *ObjectBuilder::buildUnion(Type *type) {
  auto unionObj = new Union(type);
  
  return unionObj;
}

} // extern "C"
