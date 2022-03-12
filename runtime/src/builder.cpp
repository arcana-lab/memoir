#include "builder.hpp"

using namespace objectir;

extern "C" {

/*
 * Type construction
 */
Type *objectir::getObjectType(int numFields, ...) {
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

Type *objectir::getArrayType(Type *type) {
  return new ArrayType(type);
}

Type *objectir::getUnionType(int numMembers, ...) {
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

Type *objectir::getIntegerType(uint64_t bitwidth,
                               bool isSigned) {
  return new IntegerType(bitwidth, isSigned);
}

Type *objectir::getUInt64Type() {
  return new IntegerType(64, false);
}

Type *objectir::getUInt32Type() {
  return new IntegerType(32, false);
}

Type *objectir::getUInt16Type() {
  return new IntegerType(16, false);
}

Type *objectir::getUInt8Type() {
  return new IntegerType(8, false);
}

Type *objectir::getInt64Type() {
  return new IntegerType(64, true);
}

Type *objectir::getInt32Type() {
  return new IntegerType(32, true);
}

Type *objectir::getInt16Type() {
  return new IntegerType(16, true);
}

Type *objectir::getInt8Type() {
  return new IntegerType(8, true);
}

Type *objectir::getBooleanType() {
  return new IntegerType(1, false);
}

Type *objectir::getFloatType() {
  return new FloatType();
}

Type *objectir::getDoubleType() {
  return new DoubleType();
}

/*
 * Object construction
 */
Object *objectir::buildObject(Type *type) {
  auto obj = new Object(type);

  return obj;
}

Object *objectir::buildArray(Type *type, uint64_t length) {
  auto array = new Array(type, length);

  return array;
}

} // extern "C"
