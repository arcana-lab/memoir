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

//#ifdef __cplusplus
namespace objectir {
extern "C" {
//#endif

/*
 * Type construction
 */
__attribute__((noinline)) Type *getObjectType(int numFields,
                                              ...);
__attribute__((noinline)) Type *nameObjectType(
    char *name,
    int numFields,
    ...);

__attribute__((noinline)) Type *getArrayType(
    Type *elementType);
__attribute__((noinline)) Type *nameArrayType(
    char *name,
    Type *elementType);

__attribute__((noinline)) Type *getUnionType(int numMembers,
                                             ...);
__attribute__((noinline)) Type *nameUnionType(
    char *name,
    int numMembers,
    ...);

/*
 * Primitive Types
 */
__attribute__((noinline)) Type *getIntegerType(
    uint64_t bitwidth,
    bool isSigned);
__attribute__((noinline)) Type *getUInt64Type();
__attribute__((noinline)) Type *getUInt32Type();
__attribute__((noinline)) Type *getUInt16Type();
__attribute__((noinline)) Type *getUInt8Type();
__attribute__((noinline)) Type *getInt64Type();
__attribute__((noinline)) Type *getInt32Type();
__attribute__((noinline)) Type *getInt16Type();
__attribute__((noinline)) Type *getInt8Type();
__attribute__((noinline)) Type *getBooleanType();
__attribute__((noinline)) Type *getFloatType();
__attribute__((noinline)) Type *getDoubleType();
__attribute__((noinline)) Type *getPointerType(
    Type *containedType);

/*
 * Named Types
 */
__attribute__((noinline)) Type *getNamedType(char *name);

/*
 * Object construction
 */
__attribute__((noinline)) Object *buildObject(Type *type);
__attribute__((noinline)) Array *buildArray(
    Type *type,
    uint64_t length);
__attribute__((noinline)) Union *buildUnion(Type *type);

/*
 * Object destruction
 */
__attribute__((noinline)) void deleteObject(Object *obj);

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
 * Type checking
 */
__attribute__((noinline)) bool assertType(Type *type,
                                          Object *object);
__attribute__((noinline)) bool assertFieldType(
    Type *type,
    Field *field);

__attribute__((noinline)) bool setReturnType(Type *type);

/*
 * Field accesses
 */
// Unsigned integer access
__attribute__((noinline)) void writeUInt64(Field *field,
                                           uint64_t value);
__attribute__((noinline)) void writeUInt32(Field *field,
                                           uint32_t value);
__attribute__((noinline)) void writeUInt16(Field *field,
                                           uint16_t value);
__attribute__((noinline)) void writeUInt8(Field *field,
                                          uint8_t value);

// Signed integer access
__attribute__((noinline)) void writeInt64(Field *field,
                                          int64_t value);
__attribute__((noinline)) void writeInt32(Field *field,
                                          int32_t value);
__attribute__((noinline)) void writeInt16(Field *field,
                                          int16_t value);
__attribute__((noinline)) void writeInt8(Field *field,
                                         int8_t value);

// Floating point access
__attribute__((noinline)) void writeFloat(Field *field,
                                          float value);
__attribute__((noinline)) void writeDouble(Field *field,
                                           double value);

// Pointer access
__attribute__((noinline)) void writeObject(Field *field,
                                           Object *object);

// Unsigned integer access
__attribute__((noinline)) uint64_t readUInt64(Field *field);
__attribute__((noinline)) uint32_t readUInt32(Field *field);
__attribute__((noinline)) uint16_t readUInt16(Field *field);
__attribute__((noinline)) uint8_t readUInt8(Field *field);

// Signed integer access
__attribute__((noinline)) int64_t readInt64(Field *field);
__attribute__((noinline)) int32_t readInt32(Field *field);
__attribute__((noinline)) int16_t readInt16(Field *field);
__attribute__((noinline)) int8_t readInt8(Field *field);

// Floating point access
__attribute__((noinline)) float readFloat(Field *field);
__attribute__((noinline)) double readDouble(Field *field);

// Pointer access
__attribute__((noinline)) Object *readObject(Field *field);
__attribute__((noinline)) Object *readPointer(Field *field);
__attribute__((noinline)) void writePointer(Field *field,
                                            Object *value);

  
//#ifdef __cplusplus
} // extern "C"
} // namespace objectir
//#endif
