#ifndef MEMOIR_IR_TYPECHECK_H
#define MEMOIR_IR_TYPECHECK_H
#pragma once

#include <iostream>
#include <string>

// LLVM
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"

// MEMOIR
#include "memoir/passes/Passes.hpp"

#include "memoir/ir/InstVisitor.hpp"
#include "memoir/ir/Types.hpp"

#include "memoir/support/InternalDatatypes.hpp"

#include "memoir/utility/FunctionNames.hpp"

/*
 * A simple analysis to summarize the Types present in a program.
 *
 * Author(s): Tommy McMichen
 * Created: July 5, 2022
 */

namespace llvm::memoir {

/**
 * A type variable used for unification.
 */
struct TypeVariable : public Type {
public:
  using TypeID = uint64_t;

  // Constructor
  TypeVariable(TypeID id) : Type(TypeCode::OTHER), id(id) {}

  // Equality.
  bool operator==(Type &T) const {
    if (auto *tvar = dyn_cast<TypeVariable>(&T)) {
      return tvar->id == this->id;
    }
    return false;
  }

  // This class will only be used in the context of the base types and
  // itself, so it is the only one that follows "other".
  static bool classof(const Type *t) {
    return (t->getCode() == TypeCode::OTHER);
  }

  std::string toString(std::string indent = "") const override {
    return "typevar(" + std::to_string(this->id) + ")";
  }

protected:
  TypeID id;
}; // namespace llvm::memoir

/*
 * Type Analysis
 *
 * Top level entry for MemOIR type analysis
 *
 * This type analysis provides basic information about MemOIR
 *   types defined in the program.
 */
class TypeChecker : public llvm::memoir::InstVisitor<TypeChecker, Type *> {
  friend class llvm::memoir::InstVisitor<TypeChecker, Type *>;
  friend class llvm::InstVisitor<TypeChecker, Type *>;

public:
  /**
   * Analyze a MEMOIR instruction, getting its MEMOIR type.
   *
   * @param V LLVM Value to analyze
   * @returns a pointer to the value's type, or NULL if it failed.
   */
  static Type *type_of(MemOIRInst &I);

  /**
   * Analyze an LLVM value, getting its MEMOIR type, if it exists.
   *
   * @param V LLVM Value to analyze
   * @returns a pointer to the value's type, or NULL if it is not a MEMOIR
   * variable.
   */
  static Type *type_of(llvm::Value &V);

protected:
  // Union find data structure for type bindings.
  TypeVariable &new_type_variable();
  Type *find(Type *T);
  Type *unify(Type *T, Type *U);
  map<TypeVariable *, Type *> type_bindings;
  TypeVariable::TypeID current_id;

  // Variable bindings.
  map<llvm::Value *, TypeVariable *> value_bindings;

  // Analysis functions.
  Type *analyze(MemOIRInst &I);
  Type *analyze(llvm::Value &V);

  // Visitor functions
  //// Base case
  Type *visitInstruction(llvm::Instruction &I);
  //// LLVM instructions
  Type *visitArgument(llvm::Argument &A);
  Type *visitLoadInst(llvm::LoadInst &I);
  Type *visitPHINode(llvm::PHINode &I);
  Type *visitExtractValueInst(llvm::ExtractValueInst &I);
  Type *visitLLVMCallInst(llvm::CallInst &I);
  //// Type instructions
  Type *visitUInt64TypeInst(UInt64TypeInst &I);
  Type *visitUInt32TypeInst(UInt32TypeInst &I);
  Type *visitUInt16TypeInst(UInt16TypeInst &I);
  Type *visitUInt8TypeInst(UInt8TypeInst &I);
  Type *visitUInt2TypeInst(UInt2TypeInst &I);
  Type *visitInt64TypeInst(Int64TypeInst &I);
  Type *visitInt32TypeInst(Int32TypeInst &I);
  Type *visitInt16TypeInst(Int16TypeInst &I);
  Type *visitInt8TypeInst(Int8TypeInst &I);
  Type *visitInt2TypeInst(Int2TypeInst &I);
  Type *visitBoolTypeInst(BoolTypeInst &I);
  Type *visitFloatTypeInst(FloatTypeInst &I);
  Type *visitDoubleTypeInst(DoubleTypeInst &I);
  Type *visitPointerTypeInst(PointerTypeInst &I);
  Type *visitVoidTypeInst(VoidTypeInst &I);
  Type *visitReferenceTypeInst(ReferenceTypeInst &I);
  Type *visitDefineStructTypeInst(DefineStructTypeInst &I);
  Type *visitStructTypeInst(StructTypeInst &I);
  Type *visitStaticTensorTypeInst(StaticTensorTypeInst &I);
  Type *visitTensorTypeInst(TensorTypeInst &I);
  Type *visitAssocArrayTypeInst(AssocArrayTypeInst &I);
  Type *visitSequenceTypeInst(SequenceTypeInst &I);
  //// Allocation instructions
  Type *visitStructAllocInst(StructAllocInst &I);
  Type *visitTensorAllocInst(TensorAllocInst &I);
  Type *visitAssocArrayAllocInst(AssocArrayAllocInst &I);
  Type *visitSequenceAllocInst(SequenceAllocInst &I);
  //// Access instructions
  Type *visitReadInst(ReadInst &I);
  Type *visitStructReadInst(StructReadInst &I);
  Type *visitGetInst(GetInst &I);
  Type *visitStructGetInst(StructGetInst &I);
  Type *visitWriteInst(WriteInst &I);
  //// SSA operations
  Type *visitUsePHIInst(UsePHIInst &I);
  Type *visitDefPHIInst(DefPHIInst &I);
  Type *visitArgPHIInst(ArgPHIInst &I);
  Type *visitRetPHIInst(RetPHIInst &I);
  //// SSA collection operations
  Type *visitInsertInst(InsertInst &I);
  Type *visitRemoveInst(RemoveInst &I);
  Type *visitSwapInst(SwapInst &I);
  Type *visitCopyInst(CopyInst &I);
  Type *visitFoldInst(FoldInst &I);
  //// SSA assoc operations
  Type *visitAssocHasInst(AssocHasInst &I);
  Type *visitAssocKeysInst(AssocKeysInst &I);
  Type *visitAssocRemoveInst(AssocRemoveInst &I);
  Type *visitAssocInsertInst(AssocInsertInst &I);

  // Constructor.
  TypeChecker();

  // Destructor
  ~TypeChecker();

  // This class is not cloneable nor assignable
  TypeChecker(TypeChecker &other) = delete;
  void operator=(const TypeChecker &) = delete;
};

} // namespace llvm::memoir

#endif // MEMOIR_ANALYSIS_TYPES_H
