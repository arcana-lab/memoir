#ifndef COMMON_FUNCTIONNAMES_H
#define COMMON_FUNCTIONNAMES_H
#pragma once

#include <string>

#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"

#include "memoir/support/InternalDatatypes.hpp"

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
#define MUT_FUNC(name) mut__##name

/*
 * Enum of MemOIR functions
 */
enum MemOIR_Func {
  BEGIN_MEMOIR,
#define HANDLE_INST(ENUM, FUNC, CLASS) ENUM,
#include "memoir/ir/Instructions.def"
  END_MEMOIR,
  BEGIN_MUT,
#define HANDLE_INST(ENUM, FUNC, CLASS) ENUM,
#include "memoir/ir/MutOperations.def"
  END_MUT,
  NONE
};

std::ostream &operator<<(std::ostream &os, MemOIR_Func Enum);
llvm::raw_ostream &operator<<(llvm::raw_ostream &os, MemOIR_Func Enum);

class FunctionNames {
public:
  /*
   * Utility functions
   */
  static bool is_memoir_call(llvm::Function &function);

  static bool is_memoir_call(llvm::CallInst &call_inst);

  static bool is_mut_call(llvm::Function &function);

  static bool is_mut_call(llvm::CallInst &call_inst);

  static MemOIR_Func get_memoir_enum(llvm::Function &function);

  static MemOIR_Func get_memoir_enum(llvm::CallInst &call_inst);

  static llvm::Function *get_memoir_function(llvm::Module &M,
                                             MemOIR_Func function_enum);

  static bool is_type(MemOIR_Func function_enum);

  static bool is_primitive_type(MemOIR_Func function_enum);

  static bool is_object_type(MemOIR_Func function_enum);

  static bool is_reference_type(MemOIR_Func function_enum);

  static bool is_allocation(MemOIR_Func function_enum);

  static bool is_def(MemOIR_Func function_enum);

  static bool is_access(MemOIR_Func function_enum);

  static bool is_read(MemOIR_Func function_enum);

  static bool is_write(MemOIR_Func function_enum);

  static bool is_get(MemOIR_Func function_enum);

  static bool is_mutator(MemOIR_Func function_enum);

  static bool is_mutator(llvm::Module &M, MemOIR_Func function_enum);

  static bool is_mutator(llvm::Function &F);
};

} // namespace llvm::memoir

#endif
