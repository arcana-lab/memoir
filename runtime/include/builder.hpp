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

#include "objects.hpp"
#include "types.hpp"

namespace objectir {

/*
 * Type construction
 */
extern "C" __attribute__((noinline)) Type *getObjectType(int numFields, ...);
extern "C" __attribute__((noinline)) Type *getArrayType(Type *type);
extern "C" __attribute__((noinline)) Type *getIntegerType(uint64_t bitwidth,
                                                            bool isSigned);
extern "C" __attribute__((noinline)) Type *getUInt64Type();
extern "C" __attribute__((noinline)) Type *getUInt32Type();
extern "C" __attribute__((noinline)) Type *getUInt16Type();
extern "C" __attribute__((noinline)) Type *getUInt8Type();
extern "C" __attribute__((noinline)) Type *getInt64Type();
extern "C" __attribute__((noinline)) Type *getInt32Type();
extern "C" __attribute__((noinline)) Type *getInt16Type();
extern "C" __attribute__((noinline)) Type *getInt8Type();
extern "C" __attribute__((noinline)) Type *getBooleanType();
extern "C" __attribute__((noinline)) Type *getFloatType();
extern "C" __attribute__((noinline)) Type *getDoubleType();

/*
 * Object construction
 */
extern "C" __attribute__((noinline)) Object *buildObject(Type *type);
extern "C" __attribute__((noinline)) Object *buildArray(Type *type,
                                                        uint64_t length);

} // namespace objectir
