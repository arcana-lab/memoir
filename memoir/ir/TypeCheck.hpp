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

#include "memoir/support/DataTypes.hpp"

#include "memoir/utility/FunctionNames.hpp"

/*
 * A simple analysis to summarize the Types present in a program.
 *
 * Author(s): Tommy McMichen
 * Created: July 5, 2022
 */

namespace memoir {

/*
 * Type Analysis
 *
 * Top level entry for MemOIR type analysis
 *
 * This type analysis provides basic information about MemOIR
 *   types defined in the program.
 */
class TypeChecker : public memoir::InstVisitor<TypeChecker, Type *> {
  friend class memoir::InstVisitor<TypeChecker, Type *>;
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
  Option<Type *> unify(Type *T, Type *U);
  Map<TypeVariable *, Type *> type_bindings;
  TypeVariable::TypeID current_id;

  // Variable bindings.
  Map<llvm::Value *, TypeVariable *> value_bindings;

  // Analysis functions.
  Type *analyze(MemOIRInst &I);
  Type *analyze(llvm::Value &V);

  // Helper functions.
  Type *nested_type(AccessInst &access);

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
  Type *visitTupleTypeInst(TupleTypeInst &I);
  Type *visitArrayTypeInst(ArrayTypeInst &I);
  Type *visitAssocArrayTypeInst(AssocArrayTypeInst &I);
  Type *visitSequenceTypeInst(SequenceTypeInst &I);
  // Type *visitDefineTypeInst(DefineTypeInst &I);
  // Type *visitLookupTypeInst(LookupTypeInst &I);
  //// Allocation instructions
  Type *visitAllocInst(AllocInst &I);
  //// Access instructions
  Type *visitAccessInst(AccessInst &I);
  Type *visitReadInst(ReadInst &I);
  Type *visitGetInst(GetInst &I);
  Type *visitHasInst(HasInst &I);
  Type *visitCopyInst(CopyInst &I);
  Type *visitKeysInst(KeysInst &I);
  Type *visitFoldInst(FoldInst &I);
  //// Update instructions
  Type *visitUpdateInst(UpdateInst &I);
  //// SSA operations
  Type *visitUsePHIInst(UsePHIInst &I);
  Type *visitRetPHIInst(RetPHIInst &I);

  // Constructor.
  TypeChecker();

  // Destructor
  ~TypeChecker();

  // This class is not cloneable nor assignable
  TypeChecker(TypeChecker &other) = delete;
  void operator=(const TypeChecker &) = delete;
};

} // namespace memoir

#endif // MEMOIR_ANALYSIS_TYPES_H
