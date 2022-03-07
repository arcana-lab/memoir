#include "builder.hpp"

namespace objectir {
extern "C" {

/*
 * Type construction
 */
Type *buildObjectType(int numFields, ...) {
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

Type *buildIntegerType(uint64_t bitwidth, bool isSigned) {
  return new IntegerType(bitwidth, isSigned);
}

Type *buildUInt64Type() {
  return new IntegerType(64, false);
}

Type *buildUInt32Type() {
  return new IntegerType(32, false);
}

Type *buildUInt16Type() {
  return new IntegerType(16, false);
}

Type *buildUInt8Type() {
  return new IntegerType(8, false);
}

Type *buildInt64Type() {
  return new IntegerType(64, true);
}

Type *buildInt32Type() {
  return new IntegerType(32, true);
}

Type *buildInt16Type() {
  return new IntegerType(16, true);
}

Type *buildInt8Type() {
  return new IntegerType(8, true);
}

Type *buildBooleanType() {
  return new IntegerType(1, false);
}

Type *buildFloatType() {
  return new IntegerType(bitwidth, isSigned);
}

Type *buildDoubleType() {
  return new IntegerType(bitwidth, isSigned);
}

/*
 * Object construction
 */
Object *buildObject(Type *type) {}
Object *buildArray(Type *type, uint64_t length) {}
} // extern "C"
} // namespace objectir
