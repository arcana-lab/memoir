#ifndef COMMON_TYPEANALYSIS_H
#define COMMON_TYPEANALYSIS_H
#pragma once

#include <iostream>
#include <string>

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"

#include "memoir/ir/InstVisitor.hpp"
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
class TypeAnalysis : InstVisitor<TypeAnalysis, TypeAnalysis::RetTy> {
public:
  using RetTy = std::add_pointer_t<Type>;

  /*
   * Singleton access
   */
  static TypeAnalysis &get();

  static void invalidate();

  /*
   * Query the Type Summary for the given LLVM Value
   */
  RetTy getType(llvm::Value &value);

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

  /*
   * Borrowed state
   */
  map<llvm::Value *, RetTy *> value_to_type;

  /*
   * Internal helper functions
   */
  RetTy findExisting(llvm::Value &V);
  RetTy findExisting(MemOIRInst &I);
  void memoize(llvm::Value &V, Type &T);
  void memoize(MemOIRInst &I, Type &T);

  /*
   * Visitor functions
   */
  RetTy visitIntegerTypeInst(IntegerTypeInst &I);
  RetTy visitFloatTypeInst(FloatTypeInst &I);
  RetTy visitDoubleTypeInst(DoubleTypeInst &I);
  RetTy visitPointerTypeInst(PointerTypeInst &I);
  RetTy visitReferenceTypeInst(ReferenceTypeInst &I);
  RetTy visitDefineStructTypeInst(DefineStructTypeInst &I);
  RetTy visitStructTypeInst(StructTypeInst &I);
  RetTy visitStaticTensorTypeInst(StaticTensorTypeInst &I);
  RetTy visitTensorTypeInst(TensorTypeInst &I);
  RetTy visitAssocArrayTypeInst(AssocArrayTypeInst &I);
  RetTy visitSequenceTypeInst(SequenceTypeInst &I);
  RetTy visitStructAllocInst(StructAllocInst &I);
  RetTy visitTensorAllocInst(TensorAllocInst &I);
  RetTy visitAssocArrayAllocInst(AssocArrayAllocInst &I);
  RetTy visitSequenceAllocInst(SequenceAllocInst &I);
  RetTy visitAssertStructTypeInst(AssertStructTypeInst &I);
  RetTy visitAssertCollectionTypeInst(AssertCollectionTypeInst &I);
  RetTy visitReturnTypeInst(ReturnTypeInst &I);
  RetTy visitLLVMCallInst(llvm::CallInst &I);
  RetTy visitLoadInst(llvm::LoadInst &I);

  /*
   * Private constructor and logistics
   */
  TypeAnalysis();

  void invalidate();
};

} // namespace llvm::memoir

#endif // COMMON_TYPES_H
