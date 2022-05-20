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

#define __OBJECTIR_ATTR                                    \
  __declspec(noalias) __attribute__((nothrow))             \
      __attribute__((noinline))
#define __ALLOC_ATTR __declspec(allocator)

/*
 * Type construction
 */
__OBJECTIR_ATTR Type *getObjectType(int numFields, ...);
__OBJECTIR_ATTR Type *nameObjectType(char *name,
                                     int numFields,
                                     ...);

__OBJECTIR_ATTR Type *getArrayType(Type *elementType);
__OBJECTIR_ATTR Type *nameArrayType(char *name,
                                    Type *elementType);

__OBJECTIR_ATTR Type *getUnionType(int numMembers, ...);
__OBJECTIR_ATTR Type *nameUnionType(char *name,
                                    int numMembers,
                                    ...);

/*
 * Primitive Types
 */
__OBJECTIR_ATTR Type *getIntegerType(uint64_t bitwidth,
                                     bool isSigned);
__OBJECTIR_ATTR Type *getUInt64Type();
__OBJECTIR_ATTR Type *getUInt32Type();
__OBJECTIR_ATTR Type *getUInt16Type();
__OBJECTIR_ATTR Type *getUInt8Type();
__OBJECTIR_ATTR Type *getInt64Type();
__OBJECTIR_ATTR Type *getInt32Type();
__OBJECTIR_ATTR Type *getInt16Type();
__OBJECTIR_ATTR Type *getInt8Type();
__OBJECTIR_ATTR Type *getBooleanType();
__OBJECTIR_ATTR Type *getFloatType();
__OBJECTIR_ATTR Type *getDoubleType();
__OBJECTIR_ATTR Type *getPointerType(Type *containedType);

/*
 * Named Types
 */
__OBJECTIR_ATTR Type *getNamedType(char *name);

/*
 * Object construction
 */
__ALLOC_ATTR __OBJECTIR_ATTR Object *buildObject(
    Type *type);
__ALLOC_ATTR __OBJECTIR_ATTR Array *buildArray(
    Type *type,
    uint64_t length);
__OBJECTIR_ATTR Union *buildUnion(Type *type);

/*
 * Object destruction
 */
__OBJECTIR_ATTR void deleteObject(Object *obj);

/*
 * Object accesses
 */
__OBJECTIR_ATTR Field *getObjectField(Object *object,
                                      uint64_t fieldNo);

/*
 * Array accesses
 */
__OBJECTIR_ATTR Field *getArrayElement(Array *array,
                                       uint64_t index);

/*
 * Union accesses
 */
__OBJECTIR_ATTR Field *getUnionMember(Union *unionObj,
                                      uint64_t index);

/*
 * Type checking
 */
__OBJECTIR_ATTR bool assertType(Type *type, Object *object);
__OBJECTIR_ATTR bool assertFieldType(Type *type,
                                     Field *field);

__OBJECTIR_ATTR bool setReturnType(Type *type);

/*
 * Field accesses
 */
// Unsigned integer access
__OBJECTIR_ATTR void writeUInt64(Field *field,
                                 uint64_t value);
__OBJECTIR_ATTR void writeUInt32(Field *field,
                                 uint32_t value);
__OBJECTIR_ATTR void writeUInt16(Field *field,
                                 uint16_t value);
__OBJECTIR_ATTR void writeUInt8(Field *field,
                                uint8_t value);

// Signed integer access
__OBJECTIR_ATTR void writeInt64(Field *field,
                                int64_t value);
__OBJECTIR_ATTR void writeInt32(Field *field,
                                int32_t value);
__OBJECTIR_ATTR void writeInt16(Field *field,
                                int16_t value);
__OBJECTIR_ATTR void writeInt8(Field *field, int8_t value);

// Floating point access
__OBJECTIR_ATTR void writeFloat(Field *field, float value);
__OBJECTIR_ATTR void writeDouble(Field *field,
                                 double value);

// Pointer access
__OBJECTIR_ATTR void writeObject(Field *field,
                                 Object *object);

// Unsigned integer access
__OBJECTIR_ATTR uint64_t readUInt64(Field *field);
__OBJECTIR_ATTR uint32_t readUInt32(Field *field);
__OBJECTIR_ATTR uint16_t readUInt16(Field *field);
__OBJECTIR_ATTR uint8_t readUInt8(Field *field);

// Signed integer access
__OBJECTIR_ATTR int64_t readInt64(Field *field);
__OBJECTIR_ATTR int32_t readInt32(Field *field);
__OBJECTIR_ATTR int16_t readInt16(Field *field);
__OBJECTIR_ATTR int8_t readInt8(Field *field);

// Floating point access
__OBJECTIR_ATTR float readFloat(Field *field);
__OBJECTIR_ATTR double readDouble(Field *field);

// Pointer access
__OBJECTIR_ATTR Object *readObject(Field *field);
__OBJECTIR_ATTR Object *readPointer(Field *field);
__OBJECTIR_ATTR void writePointer(Field *field,
                                  Object *value);

//#ifdef __cplusplus
} // extern "C"
} // namespace objectir
//#endif
