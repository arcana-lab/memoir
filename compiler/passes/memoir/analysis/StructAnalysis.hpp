#ifndef COMMON_STRUCTANALYSIS_H
#define COMMON_STRUCTANALYSIS_H
#pragma once

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"

#include "memoir/support/Assert.hpp"

#include "memoir/support/InternalDatatypes.hpp"

#include "memoir/ir/InstVisitor.hpp"
#include "memoir/ir/Instructions.hpp"
#include "memoir/ir/Structs.hpp"

namespace llvm::memoir {

class StructAnalysis
  : public llvm::memoir::InstVisitor<StructAnalysis, Struct *> {
  friend class llvm::memoir::InstVisitor<StructAnalysis, Struct *>;
  friend class llvm::InstVisitor<StructAnalysis, Struct *>;

public:
  static StructAnalysis &get();

  static Struct *analyze(llvm::Value &V);

  static void invalidate();

  /*
   * Analysis
   */
  Struct *getStruct(llvm::Value &V);

  /*
   * This class is not clonable nor assignable.
   */
  StructAnalysis(StructAnalysis &other) = delete;
  void operator=(const StructAnalysis &other) = delete;

protected:
  /*
   * Owned state
   */
  map<llvm::Value *, Struct *> value_to_struct;

  /*
   * Borrowed state
   */

  /*
   * Internal helper functions
   */
  Struct *findExisting(llvm::Value &V);
  Struct *findExisting(MemOIRInst &I);
  void memoize(llvm::Value &V, Struct *S);
  void memoize(MemOIRInst &I, Struct *S);

  /*
   * Visitor functions
   */
  Struct *visitInstruction(llvm::Instruction &I);
  Struct *visitStructAllocInst(StructAllocInst &I);
  Struct *visitStructGetInst(StructGetInst &I);
  Struct *visitIndexGetInst(IndexGetInst &I);
  Struct *visitAssocGetInst(AssocGetInst &I);
  Struct *visitReadInst(ReadInst &I);
  Struct *visitLLVMCallInst(llvm::CallInst &I);
  Struct *visitPHINode(llvm::PHINode &I);
  Struct *visitReturnInst(llvm::ReturnInst &I);
  Struct *visitArgument(llvm::Argument &A);

  /*
   * Private constructor and logistics
   */
  StructAnalysis();

  void _invalidate();

  static StructAnalysis *SA;
};

} // namespace llvm::memoir

#endif // COMMON_STRUCTANALYSIS_H
