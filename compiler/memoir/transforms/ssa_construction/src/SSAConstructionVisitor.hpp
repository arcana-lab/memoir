#ifndef MEMOIR_TRANSFORMS_SSACONSTRUCTIONVISITOR_H
#define MEMOIR_TRANSFORMS_SSACONSTRUCTIONVISITOR_H

#include "llvm/IR/CFG.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"

#include "memoir/ir/Builder.hpp"
#include "memoir/ir/InstVisitor.hpp"
#include "memoir/ir/Instructions.hpp"

#include "llvm/ADT/MapVector.h"

namespace llvm::memoir {

using ReachingDefMapTy = map<llvm::Value *, llvm::Value *>;

struct SSAConstructionStats {
  using CountTy = uint32_t;
};

class SSAConstructionVisitor
  : public llvm::memoir::InstVisitor<SSAConstructionVisitor, void> {
  friend class llvm::memoir::InstVisitor<SSAConstructionVisitor, void>;
  friend class llvm::InstVisitor<SSAConstructionVisitor, void>;

public:
  SSAConstructionVisitor(llvm::DominatorTree &DT,
                         ordered_set<llvm::Value *> memoir_names,
                         map<llvm::PHINode *, llvm::Value *> inserted_phis,
                         SSAConstructionStats *stats = nullptr,
                         bool construct_use_phis = false);

  // LLVM operations
  void visitInstruction(llvm::Instruction &I);
  void visitPHINode(llvm::PHINode &I);
  void visitLLVMCallInst(llvm::CallInst &I);
  void visitReturnInst(llvm::ReturnInst &I);

  // SSA operations
  void visitUsePHIInst(UsePHIInst &I);
  void visitRetPHIInst(RetPHIInst &I);

  void visitFoldInst(FoldInst &I);

  // MUT Operations

  // Access operations
  void visitHasInst(HasInst &I);
  void visitReadInst(ReadInst &I);
  void visitGetInst(GetInst &I);

  // Update operations
  void visitMutWriteInst(MutWriteInst &I);
  void visitMutInsertInst(MutInsertInst &I);
  void visitMutRemoveInst(MutRemoveInst &I);
  void visitMutClearInst(MutClearInst &I);

  llvm::Value *update_reaching_definition(llvm::Value *variable,
                                          MemOIRInst &program_point);
  llvm::Value *update_reaching_definition(llvm::Value *variable,
                                          llvm::Instruction &program_point);
  llvm::Value *update_reaching_definition(llvm::Value *variable,
                                          llvm::Instruction *program_point);

  void set_reaching_definition(llvm::Value *variable,
                               llvm::Value *reaching_definition);
  void set_reaching_definition(llvm::Value *variable,
                               MemOIRInst *reaching_definition);
  void set_reaching_definition(MemOIRInst *variable,
                               llvm::Value *reaching_definition);
  void set_reaching_definition(MemOIRInst *variable,
                               MemOIRInst *reaching_definition);

  void mark_for_cleanup(llvm::Instruction &I);
  void mark_for_cleanup(MemOIRInst &I);

  void prepare_keywords(MemOIRBuilder &builder,
                        MemOIRInst &I,
                        Type *element_type = nullptr);

  void cleanup();

protected:
  MemOIRBuilder *builder;
  ReachingDefMapTy reaching_definitions;
  llvm::DominatorTree &DT;
  set<llvm::Instruction *> instructions_to_delete;
  map<llvm::PHINode *, llvm::Value *> inserted_phis;

  // Options.
  bool construct_use_phis;

  // Statistics
  SSAConstructionStats *stats;
};

bool use_is_mutating(llvm::Use &use, bool construct_use_phis = false);

} // namespace llvm::memoir

#endif // MEMOIR_TRANSFORMS_SSACONSTRUCTIONVISITOR_H
