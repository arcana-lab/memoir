#ifndef MEMOIR_MEMOIR_H
#define MEMOIR_MEMOIR_H
#pragma once

/*
 * Object representation recognizable by LLVM IR
 * This file describes the API for building and
 * accessing object-ir objects, fields and types
 *
 * Author(s): Tommy McMichen
 * Created: Mar 4, 2022
 */

#include "objects.h"
#include "types.h"

namespace memoir {
extern "C" {

#define __RUNTIME_ATTR                                                         \
  __declspec(noalias) __attribute__((nothrow)) __attribute__((noinline))
#define __ALLOC_ATTR __declspec(allocator)

/*
 * Struct Types
 */
__RUNTIME_ATTR Type *defineStructType(const char *name, int num_fields, ...);
__RUNTIME_ATTR Type *StructType(const char *name);

/*
 * Complex Types
 */
__RUNTIME_ATTR Type *TensorType(Type *element_type,
                                uint64_t num_dimensions,
                                ...);
__RUNTIME_ATTR Type *ReferenceType(Type *referenced_type);

/*
 * Primitive Types
 */
__RUNTIME_ATTR Type *IntegerType(uint64_t bitwidth, bool is_signed);
__RUNTIME_ATTR Type *UInt64Type();
__RUNTIME_ATTR Type *UInt32Type();
__RUNTIME_ATTR Type *UInt16Type();
__RUNTIME_ATTR Type *UInt8Type();
__RUNTIME_ATTR Type *Int64Type();
__RUNTIME_ATTR Type *Int32Type();
__RUNTIME_ATTR Type *Int16Type();
__RUNTIME_ATTR Type *Int8Type();
__RUNTIME_ATTR Type *BoolType();
__RUNTIME_ATTR Type *FloatType();
__RUNTIME_ATTR Type *DoubleType();

/*
 * Object construction
 */
__ALLOC_ATTR __RUNTIME_ATTR Object *allocateStruct(Type *type);
__ALLOC_ATTR __RUNTIME_ATTR Object *allocateTensor(Type *element_type,
                                                   uint64_t num_dimensions,
                                                   ...);

/*
 * Object accesses
 */
__RUNTIME_ATTR Field *getStructField(Object *object, uint64_t field_index);
__RUNTIME_ATTR Field *getTensorElement(Object *tensor, ...);
/*
 * Object destruction
 */
__RUNTIME_ATTR void deleteObject(Object *object);

/*
 * Type checking and function signatures
 */
__RUNTIME_ATTR bool assertType(Type *type, Object *object);
__RUNTIME_ATTR bool setReturnType(Type *type);

/*
 * Field accesses
 */
// Unsigned integer access
__RUNTIME_ATTR void writeUInt64(Field *field, uint64_t value);
__RUNTIME_ATTR void writeUInt32(Field *field, uint32_t value);
__RUNTIME_ATTR void writeUInt16(Field *field, uint16_t value);
__RUNTIME_ATTR void writeUInt8(Field *field, uint8_t value);
__RUNTIME_ATTR uint64_t readUInt64(Field *field);
__RUNTIME_ATTR uint32_t readUInt32(Field *field);
__RUNTIME_ATTR uint16_t readUInt16(Field *field);
__RUNTIME_ATTR uint8_t readUInt8(Field *field);

// Signed integer access
__RUNTIME_ATTR void writeInt64(Field *field, int64_t value);
__RUNTIME_ATTR void writeInt32(Field *field, int32_t value);
__RUNTIME_ATTR void writeInt16(Field *field, int16_t value);
__RUNTIME_ATTR void writeInt8(Field *field, int8_t value);
__RUNTIME_ATTR int64_t readInt64(Field *field);
__RUNTIME_ATTR int32_t readInt32(Field *field);
__RUNTIME_ATTR int16_t readInt16(Field *field);
__RUNTIME_ATTR int8_t readInt8(Field *field);

// General integer access
// NOTE: you must cast the output of these function calls to the
//   actual integer type you wish to use. These functions use uint64_t
//   to be more general purpose and easier to compile for since they
//   should be rare.
__RUNTIME_ATTR void writeInteger(Field *field, uint64_t value);
__RUNTIME_ATTR uint64_t readInteger(Field *field);

// Boolean access
__RUNTIME_ATTR void writeBool(Field *field, bool value);
__RUNTIME_ATTR bool readBool(Field *field);

// Floating point access
__RUNTIME_ATTR void writeFloat(Field *field, float value);
__RUNTIME_ATTR void writeDouble(Field *field, double value);
__RUNTIME_ATTR float readFloat(Field *field);
__RUNTIME_ATTR double readDouble(Field *field);

// Nested object access
__RUNTIME_ATTR Struct *readStruct(Field *field);
__RUNTIME_ATTR Tensor *readTensor(Field *field);

// Pointer access
__RUNTIME_ATTR void writeReference(Field *field, Object *object_to_reference);
__RUNTIME_ATTR Object *readReference(Field *field);

} // extern "C"
} // namespace memoir

#endif
