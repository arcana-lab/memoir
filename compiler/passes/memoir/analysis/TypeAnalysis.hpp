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
class TypeAnalysis : public llvm::memoir::InstVisitor<TypeAnalysis, Type *> {
  friend class llvm::memoir::InstVisitor<TypeAnalysis, Type *>;
  friend class llvm::InstVisitor<TypeAnalysis, Type *>;

public:
  /*
   * Singleton access
   */
  static TypeAnalysis &get();

  static Type *analyze(llvm::Value &V);

  static void invalidate();

  /*
   * Query the Type Summary for the given LLVM Value
   */
  Type *getType(llvm::Value &value);

  /*
   * Query the Type for the given LLVM Function
   */
  Type *getReturnType(llvm::Function &F);

  /*
   * Helper functions
   */

  /*
   * This class is not cloneable nor assignable
   */
  TypeAnalysis(TypeAnalysis &other) = delete;
  void operator=(const TypeAnalysis &) = delete;

protected:
  /*
   * Owned state
   */

  /*
   * Borrowed state
   */
  map<llvm::Value *, Type *> value_to_type;
  map<llvm::PHINode *, set<Type *>> phi_edge_types;
  map<llvm::PHINode *, set<llvm::Value *>> visited_phi_edges;

  /*
   * Internal helper functions
   */
  Type *findExisting(llvm::Value &V);
  Type *findExisting(MemOIRInst &I);
  void memoize(llvm::Value &V, Type *T);
  void memoize(MemOIRInst &I, Type *T);

  /*
   * Visitor functions
   */
  Type *visitInstruction(llvm::Instruction &I);
  Type *visitUInt64TypeInst(UInt64TypeInst &I);
  Type *visitUInt32TypeInst(UInt32TypeInst &I);
  Type *visitUInt16TypeInst(UInt16TypeInst &I);
  Type *visitUInt8TypeInst(UInt8TypeInst &I);
  Type *visitInt64TypeInst(Int64TypeInst &I);
  Type *visitInt32TypeInst(Int32TypeInst &I);
  Type *visitInt16TypeInst(Int16TypeInst &I);
  Type *visitInt8TypeInst(Int8TypeInst &I);
  Type *visitBoolTypeInst(BoolTypeInst &I);
  Type *visitFloatTypeInst(FloatTypeInst &I);
  Type *visitDoubleTypeInst(DoubleTypeInst &I);
  Type *visitPointerTypeInst(PointerTypeInst &I);
  Type *visitReferenceTypeInst(ReferenceTypeInst &I);
  Type *visitDefineStructTypeInst(DefineStructTypeInst &I);
  Type *visitStructTypeInst(StructTypeInst &I);
  Type *visitStaticTensorTypeInst(StaticTensorTypeInst &I);
  Type *visitTensorTypeInst(TensorTypeInst &I);
  Type *visitAssocArrayTypeInst(AssocArrayTypeInst &I);
  Type *visitSequenceTypeInst(SequenceTypeInst &I);
  Type *visitStructAllocInst(StructAllocInst &I);
  Type *visitTensorAllocInst(TensorAllocInst &I);
  Type *visitAssocArrayAllocInst(AssocArrayAllocInst &I);
  Type *visitSequenceAllocInst(SequenceAllocInst &I);
  Type *visitReadInst(ReadInst &I);
  Type *visitGetInst(GetInst &I);
  // SSA operations
  Type *visitUsePHIInst(UsePHIInst &I);
  Type *visitDefPHIInst(DefPHIInst &I);
  // SSA sequence operations
  Type *visitJoinInst(JoinInst &I);
  Type *visitSliceInst(SliceInst &I);
  // Mut sequence operations
  Type *visitSeqInsertInst(SeqInsertInst &I);
  Type *visitSeqRemoveInst(SeqRemoveInst &I);
  Type *visitSeqAppendInst(SeqAppendInst &I);
  Type *visitSeqSwapInst(SeqSwapInst &I);
  Type *visitSeqSplitInst(SeqSplitInst &I);
  // Mut assoc operations
  Type *visitAssocHasInst(AssocHasInst &I);
  Type *visitAssocRemoveInst(AssocRemoveInst &I);
  Type *visitAssocKeysInst(AssocKeysInst &I);
  // Type checking
  Type *visitAssertStructTypeInst(AssertStructTypeInst &I);
  Type *visitAssertCollectionTypeInst(AssertCollectionTypeInst &I);
  Type *visitReturnTypeInst(ReturnTypeInst &I);
  Type *visitLLVMCallInst(llvm::CallInst &I);
  Type *visitLoadInst(llvm::LoadInst &I);
  Type *visitPHINode(llvm::PHINode &I);
  Type *visitArgument(llvm::Argument &A);

  /*
   * Private constructor and logistics
   */
  TypeAnalysis();

  void _invalidate();

  static TypeAnalysis *TA;
};

} // namespace llvm::memoir

#endif // COMMON_TYPES_H
