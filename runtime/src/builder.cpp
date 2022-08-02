#include <iostream>
#include <stdarg.h>

#include "memoir.h"

namespace memoir {

extern "C" {

/*
 * Type construction
 */
__RUNTIME_ATTR
Type *defineStructType(const char *name, int num_fields, ...) {
  std::vector<Type *> fields;

  va_list args;

  va_start(args, num_fields);

  for (int i = 0; i < num_fields; i++) {
    auto arg = va_arg(args, Type *);
    fields.push_back(arg);
  }

  va_end(args);

  auto type = StructType::define(name, fields);

  return type;
}

__RUNTIME_ATTR
Type *StructType(const char *name) {
  return StructType::get(name);
}

__RUNTIME_ATTR
Type *TensorType(Type *type, uint64_t num_dimensions) {
  return TensorType::get(type, num_dimensions);
}

__RUNTIME_ATTR
Type *IntegerType(uint64_t bitwidth, bool isSigned) {
  return IntegerType::get(bitwidth, isSigned);
}

__RUNTIME_ATTR
Type *UInt64Type() {
  return IntegerType::get(64, false);
}

__RUNTIME_ATTR
Type *UInt32Type() {
  return IntegerType::get(32, false);
}

__RUNTIME_ATTR
Type *UInt16Type() {
  return IntegerType::get(16, false);
}

__RUNTIME_ATTR
Type *UInt8Type() {
  return IntegerType::get(8, false);
}

__RUNTIME_ATTR
Type *Int64Type() {
  return IntegerType::get(64, true);
}

__RUNTIME_ATTR
Type *Int32Type() {
  return IntegerType::get(32, true);
}

__RUNTIME_ATTR
Type *Int16Type() {
  return IntegerType::get(16, true);
}

__RUNTIME_ATTR
Type *Int8Type() {
  return IntegerType::get(8, true);
}

__RUNTIME_ATTR
Type *BoolType() {
  return IntegerType::get(1, false);
}

__RUNTIME_ATTR
Type *FloatType() {
  return FloatType::get();
}

__RUNTIME_ATTR
Type *DoubleType() {
  return DoubleType::get();
}

__RUNTIME_ATTR
Type *ReferenceType(Type *referenced_type) {
  return ReferenceType::get(referenced_type);
}

/*
 * Struct allocation
 *   allocateStruct(struct type)
 */
__ALLOC_ATTR
__RUNTIME_ATTR
Object *allocateStruct(Type *type) {
  auto strct = new struct Struct(type);

  return strct;
}

/*
 * Tensor allocation
 *   allocateTensor(
 *     element type,
 *     number of dimensions,
 *     <length of dimension, ...>)
 */
__ALLOC_ATTR
__RUNTIME_ATTR
Object *allocateTensor(Type *element_type, uint64_t num_dimensions, ...) {
  std::vector<uint64_t> length_of_dimensions;

  va_list args;

  va_start(args, num_dimensions);

  for (int i = 0; i < num_dimensions; i++) {
    auto length_of_dimension = va_arg(args, uint64_t);
    length_of_dimensions.push_back(length_of_dimension);
  }

  va_end(args);

  auto tensor_type = TensorType(element_type, num_dimensions);

  auto tensor = new struct Tensor(tensor_type, length_of_dimensions);

  return tensor;
}

__RUNTIME_ATTR
void deleteObject(Object *obj) {
  delete obj;
}

} // extern "C"
} // namespace memoir
