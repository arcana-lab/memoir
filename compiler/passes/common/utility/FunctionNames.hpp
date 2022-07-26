#ifndef COMMON_FUNCTIONNAMES_H
#define COMMON_FUNCTIONNAMES_H
#pragma once

#include <string>
#include <unordered_map>

#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"

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
enum MemOIR_Func {
#define X(MemOIR_Enum, MemOIR_Str) MemOIR_Enum,
#include "FunctionNames.def"
#undef X
  NONE
};

/*
 * Utility functions
 */
bool isMemOIRCall(llvm::Function &function);

bool isMemOIRCall(llvm::CallInst &call_inst);

MemOIR_Func getMemOIREnum(llvm::Function &function);

MemOIR_Func getMemOIREnum(llvm::CallInst &call_inst);

llvm::Function *getMemOIRFunction(Module &M, MemOIR_Func function_enum);

/*
 * Mapping from MemOIR function enum to function name as
 * string and vice versa
 */
static std::unordered_map<MemOIR_Func, std::string> MemOIRToFunctionNames = {
#define X(MemOIR_Enum, MemOIR_Str) { MemOIR_Enum, MemOIR_Str },
#include "FunctionNames.def"
#undef X
};

static std::unordered_map<std::string, MemOIR_Func> FunctionNamesToMemOIR = {
#define X(MemOIR_Enum, MemOIR_Str) { MemOIR_Str, MemOIR_Enum },
#include "FunctionNames.def"
#undef X
};

} // namespace llvm::memoir

#endif
