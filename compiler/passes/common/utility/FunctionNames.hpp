#ifndef COMMON_FUNCTIONNAMES_H
#define COMMON_FUNCTIONNAMES_H
#pragma once

#include <string>

#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"

#include "common/support/InternalDatatypes.hpp"

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
 * Macro to apply the memoir prefix to the function name
 */
#define MEMOIR_FUNC(name) memoir__##name

/*
 * Enum of MemOIR functions
 */
enum MemOIR_Func {
#define HANDLE_INST(MemOIR_Enum, MemOIR_Str, _) MemOIR_Enum,
#include "FunctionNames.def"
#undef HANDLE_INST
  NONE
};

class FunctionNames {
public:
  /*
   * Utility functions
   */
  static bool is_memoir_call(llvm::Function &function);

  static bool is_memoir_call(llvm::CallInst &call_inst);

  static MemOIR_Func get_memoir_enum(llvm::Function &function);

  static MemOIR_Func get_memoir_enum(llvm::CallInst &call_inst);

  static llvm::Function *get_memoir_function(Module &M,
                                             MemOIR_Func function_enum);

  static bool is_type(MemOIR_Func function_enum);

  static bool is_primitive_type(MemOIR_Func function_enum);

  static bool is_object_type(MemOIR_Func function_enum);

  static bool is_reference_type(MemOIR_Func function_enum);

  static bool is_allocation(MemOIR_Func function_enum);

  static bool is_access(MemOIR_Func function_enum);

  static bool is_read(MemOIR_Func function_enum);

  static bool is_write(MemOIR_Func function_enum);

  static bool is_get(MemOIR_Func function_enum);

  /*
   * Mapping from MemOIR function enum to function name as
   * string and vice versa
   */
  static const map<MemOIR_Func, std::string> memoir_to_function_names;

  static const map<std::string, MemOIR_Func> function_names_to_memoir;
};

const map<MemOIR_Func, std::string> FunctionNames::memoir_to_function_names = {
#define HANDLE_INST(MemOIR_Enum, MemOIR_Str, _) { MemOIR_Enum, #MemOIR_Str },
#include "FunctionNames.def"
#undef HANDLE_INST
};

const map<std::string, MemOIR_Func> FunctionNames::function_names_to_memoir = {
#define HANDLE_INST(MemOIR_Enum, MemOIR_Str, _) { #MemOIR_Str, MemOIR_Enum },
#include "FunctionNames.def"
#undef HANDLE_INST
};

} // namespace llvm::memoir

#endif
