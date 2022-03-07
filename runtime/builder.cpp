#include "builder.hpp"

using namespace objectir;
extern "C" {

/*
 * Type construction
 */
Type *objectir::buildObjectType(int numFields, ...) {
  auto type = new Object();

  va_list args;

  va_start(args, numFields);

  for (int i = 0; i < numFields; i++) {
    auto arg = va_arg(args, Type *);
    type->fields.push_back(arg);
  }

  va_end(args);

  return type;
}

Type *objectir::buildIntegerType(uint64_t bitwidth, bool isSigned) {
  return new IntegerType(bitwidth, isSigned);
}

Type *objectir::buildUInt64Type() {
  return new IntegerType(64, false);
}

Type *objectir::buildUInt32Type() {
  return new IntegerType(32, false);
}

Type *objectir::buildUInt16Type() {
  return new IntegerType(16, false);
}

Type *objectir::buildUInt8Type() {
  return new IntegerType(8, false);
}

Type *objectir::buildInt64Type() {
  return new IntegerType(64, true);
}

Type *objectir::buildInt32Type() {
  return new IntegerType(32, true);
}

Type *objectir::buildInt16Type() {
  return new IntegerType(16, true);
}

Type *objectir::buildInt8Type() {
  return new IntegerType(8, true);
}

Type *objectir::buildBooleanType() {
  return new IntegerType(1, false);
}

Type *objectir::buildFloatType() {
  return new IntegerType(bitwidth, isSigned);
}

Type *objectir::buildDoubleType() {
  return new IntegerType(bitwidth, isSigned);
}

/*
 * Object construction
 */
Object *objectir::buildObject(Type *type) {
  auto obj = new Object(type);

  for (auto t : type->fields) {
    auto field = Field::createField(type);
  }
}

Object *objectir::buildArray(Type *type, uint64_t length) {
  auto array = new Array(type, length);
  std::vector<Field *> fields;

  for (int i = 0; i < length; i++) {
    auto elem = Field::createField(type);

    array->fields.push_back(elem);
  }
}

} // extern "C"
} // namespace objectir
