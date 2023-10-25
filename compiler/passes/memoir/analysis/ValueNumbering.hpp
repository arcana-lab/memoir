#ifndef MEMOIR_VALUENUMBERING_H
#define MEMOIR_VALUENUMBERING_H
#pragma once

/*
 * This file contains the analysis needed to perform ValueNumbering.
 *
 * Author(s): Tommy McMichen
 * Created: April 4, 2023
 */

// LLVM
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"

// MemOIR
#include "memoir/ir/InstVisitor.hpp"
#include "memoir/ir/Instructions.hpp"

#include "memoir/analysis/ValueExpression.hpp"

namespace llvm::memoir {

struct ValueExpression;

struct ValueTable {
public:
  ValueTable() {}

  ValueExpression *lookup(llvm::Value &V);
  ValueExpression *lookup(llvm::Use &V);
  bool insert(llvm::Value &V, ValueExpression &E);
  bool insert(llvm::Use &U, ValueExpression &E);

protected:
  map<llvm::Value *, ValueExpression *> table;
  map<llvm::Value *, map<llvm::Use *, ValueExpression *>> use_table;
};

class ValueNumbering
  : public llvm::memoir::InstVisitor<ValueNumbering, ValueExpression *> {
  friend class llvm::memoir::InstVisitor<ValueNumbering, ValueExpression *>;
  friend class llvm::InstVisitor<ValueNumbering, ValueExpression *>;

public:
  ValueNumbering(llvm::Module &M) : M(M) {}

  ~ValueNumbering() {}

  ValueExpression *get(llvm::Value &V);

  ValueExpression *get(llvm::Use &U);

  ValueExpression *get(MemOIRInst &I);

protected:
  // Owned state.
  ValueTable VT;

  // Borrowed state.
  llvm::Module &M;

  // Utility methods.
  ValueExpression *lookup(llvm::Value &V);
  ValueExpression *lookup(llvm::Use &U);
  ValueExpression *lookup(MemOIRInst &I);
  void insert(llvm::Value &V, ValueExpression *expr);
  void insert(llvm::Use &U, ValueExpression *expr);
  void insert(MemOIRInst &I, ValueExpression *expr);
  ValueExpression *lookupOrInsert(llvm::Value &V, ValueExpression *expr);
  ValueExpression *lookupOrInsert(llvm::Use &U, ValueExpression *expr);

  // Value visitor methods.
  ValueExpression *visitUse(llvm::Use &U);
  ValueExpression *visitValue(llvm::Value &U);
  ValueExpression *visitArgument(llvm::Argument &I);
  ValueExpression *visitConstant(llvm::Constant &C);

  // Instruction visitor methods.
  ValueExpression *visitInstruction(llvm::Instruction &I);
  ValueExpression *visitICmpInst(llvm::ICmpInst &I);
  ValueExpression *visitCastInst(llvm::CastInst &I);
  ValueExpression *visitPHINode(llvm::PHINode &I);
  ValueExpression *visitLLVMCallInst(llvm::CallInst &I);
  ValueExpression *visitSizeInst(SizeInst &I);
};
} // namespace llvm::memoir

#endif
