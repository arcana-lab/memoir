#pragma once

/*
 * Object representation recognizable by LLVM IR
 * This file describes the builder API for using
 * the object IR library
 *
 * Author(s): Tommy McMichen
 * Created: Mar 4, 2022
 */

#include <stdarg.h>
#include <stdio.h>

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
__attribute__((noinline)) Type *getArrayType(Type *type);
__attribute__((noinline)) Type *getUnionType(int numMembers,
                                             ...);
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

//#ifdef __cplusplus
} // extern "C"
} // namespace objectir
//#endif
