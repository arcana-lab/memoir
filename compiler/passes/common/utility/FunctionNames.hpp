#ifndef COMMON_FUNCTIONNAMES_H
#define COMMON_FUNCTIONNAMES_H
#pragma once

#include <string>
#include <unordered_map>

/*
 * This file provides general utilities for determining if a function is a
 * MemOIR API call based on the function name. It also provides an enumeration
 * of the available MemOIR functions. The enumeration elements and their
 * corresponding function names can be found in FunctionNames.def
 *
 * Author(s): Tommy McMichen
 * Created: July 5, 2022
 */

namespace llvm::memoir {

/*
 * Enum of MemOIR functions
 */
enum MemOIRFunc {
#define X(MemOIR_Enum, MemOIR_Str) MemOIR_Enum,
#include "FunctionNames.def"
#undef X
  NONE
};

/*
 * Utility functions
 */
bool isMemOIRCall(std::string function_name);

MemOIRFunc getMemOIRCall(std::string function_name);

/*
 * Mapping from MemOIR function enum to function name as
 * string and vice versa
 */
static std::unordered_map<ObjectIRFunc, std::string> MemOIRToFunctionNames = {
#define X(MemOIR_Enum, MemOIR_Str) { MemOIR_Enum, MemOIR_Str },
#include "FunctionNames.def"
#undef X
};

static std::unordered_map<std::string, ObjectIRFunc> FunctionNamesToMemOIR = {
#define X(MemOIR_Enum, MemOIR_Str) { MemOIR_Str, MemOIR_Enum },
#include "FunctionNames.def"
#undef X
};

} // namespace llvm::memoir

#endif
