#pragma once

/*
 * Object representation recognizable by LLVM IR
 * This file contains methods to access object-ir
 * objects, fields and types for the object-ir library.
 *
 * Author(s): Tommy McMichen
 * Created: Mar 11, 2022
 */

#include "objects.h"
#include "types.h"

#ifdef __cplusplus
namespace objectir {
extern "C" {
#endif

/*
 * Object accesses
 */
__attribute__((noinline)) Field *getObjectField(
    Object *object,
    uint64_t fieldNo);

/*
 * Array accesses
 */
__attribute__((noinline)) Field *getArrayElement(
    Array *array,
    uint64_t index);

/*
 * Union accesses
 */
__attribute__((noinline)) Field *getUnionMember(
    Union *unionObj,
    uint64_t index);

/*
 * Field accesses
 */
// Unsigned integer access
__attribute__((noinline)) void writeUInt64(
    IntegerField *field,
    uint64_t value);
__attribute__((noinline)) void writeUInt32(
    IntegerField *field,
    uint32_t value);
__attribute__((noinline)) void writeUInt16(
    IntegerField *field,
    uint16_t value);
__attribute__((noinline)) void writeUInt8(
    IntegerField *field,
    uint8_t value);

// Signed integer access
__attribute__((noinline)) void writeInt64(
    IntegerField *field,
    int64_t value);
__attribute__((noinline)) void writeInt32(
    IntegerField *field,
    int32_t value);
__attribute__((noinline)) void writeInt16(
    IntegerField *field,
    int16_t value);
__attribute__((noinline)) void writeInt8(
    IntegerField *field,
    int8_t value);

// Floating point access
__attribute__((noinline)) void writeFloat(FloatField *field,
                                          float value);
__attribute__((noinline)) void writeDouble(
    DoubleField *field,
    double value);

// Pointer access
__attribute__((noinline)) void writeObject(
    ObjectField *field,
    Object *object);

// Unsigned integer access
__attribute__((noinline)) uint64_t readUInt64(
    IntegerField *field);
__attribute__((noinline)) uint32_t readUInt32(
    IntegerField *field);
__attribute__((noinline)) uint16_t readUInt16(
    IntegerField *field);
__attribute__((noinline)) uint8_t readUInt8(
    IntegerField *field);

// Signed integer access
__attribute__((noinline)) int64_t readInt64(
    IntegerField *field);
__attribute__((noinline)) int32_t readInt32(
    IntegerField *field);
__attribute__((noinline)) int16_t readInt16(
    IntegerField *field);
__attribute__((noinline)) int8_t readInt8(
    IntegerField *field);

// Floating point access
__attribute__((noinline)) float readFloat(
    FloatField *field);
__attribute__((noinline)) double readDouble(
    DoubleField *field);

// Pointer access
__attribute__((noinline)) Object *readObject(
    ObjectField *field);

#ifdef __cplusplus
} // extern "C"
} // namespace objectir
#endif
