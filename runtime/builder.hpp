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
extern "C" Type *buildObjectType(int numFields, ...);
extern "C" Type *buildIntegerType(uint64_t bitwidth, bool isSigned);
extern "C" Type *buildUInt64Type();
extern "C" Type *buildUInt32Type();
extern "C" Type *buildUInt16Type();
extern "C" Type *buildUInt8Type();
extern "C" Type *buildInt64Type();
extern "C" Type *buildInt32Type();
extern "C" Type *buildInt16Type();
extern "C" Type *buildInt8Type();
extern "C" Type *buildBooleanType();
extern "C" Type *buildFloatType();
extern "C" Type *buildDoubleType();

/*
 * Object construction
 */
extern "C" Object *buildObject(Type *type);
extern "C" Object *buildArray(Type *type, uint64_t length);

} // namespace objectir
