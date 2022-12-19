#ifndef COMMON_TYPEANALYSIS_H
#define COMMON_TYPEANALYSIS_H
#pragma once

#include <iostream>
#include <string>

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"

#include "memoir/ir/Types.hpp"

#include "memoir/analysis/CollectionAnalysis.hpp"
#include "memoir/support/InternalDatatypes.hpp"
#include "memoir/utility/FunctionNames.hpp"

/*
 * A simple analysis to summarize the Types present in a program.
 *
 * Author(s): Tommy McMichen
 * Created: July 5, 2022
 */

namespace llvm::memoir {

/*
 * Type Analysis
 *
 * Top level entry for MemOIR type analysis
 *
 * This type analysis provides basic information about MemOIR
 *   types defined in the program.
 */
class TypeAnalysis {
public:
  /*
   * Singleton access
   */
  static TypeAnalysis &get(Module &M);

  static void invalidate(Module &M);

  /*
   * Query the Type Summary for the given LLVM Value
   */
  TypeSummary *getTypeSummary(llvm::Value &value);

  /*
   * Helper functions
   */

  /*
   * This class is not cloneable nor assignable
   */
  TypeAnalysis(TypeAnalysis &other) = delete;
  void operator=(const TypeAnalysis &) = delete;

private:
  /*
   * Passed state
   */
  Module &M;

  /*
   * Owned state
   */
  map<llvm::Value *, Type *> type_summaries;

  /*
   * Internal helper functions
   */
  Type *getMemOIRType(llvm::CallInst &call_inst);

  Type &getPrimitiveType(MemOIR_Func function_enum);

  Type &getIntegerType(llvm::CallInst &call_inst);

  Type &getReferenceType(llvm::CallInst &call_inst);

  Type &getStructType(llvm::CallInst &call_inst);

  Type &getTensorType(llvm::CallInst &call_inst);

  Type &getStaticTensorType(llvm::CallInst &call_inst);

  Type &getAssocArrayType(llvm::CallInst &call_inst);

  Type &getSequenceType(llvm::CallInst &call_inst);

  Type &defineStructType(llvm::CallInst &call_inst);

  /*
   * Private constructor and logistics
   */
  TypeAnalysis(llvm::Module &M);

  void invalidate();

  static map<llvm::Module *, TypeAnalysis *> analyses;
};

} // namespace llvm::memoir

#endif // COMMON_TYPES_H
