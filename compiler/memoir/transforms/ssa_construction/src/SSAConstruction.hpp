#include "llvm/IR/CFG.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"

#include "noelle/core/Noelle.hpp"

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
  // SSA operations
  void visitUsePHIInst(UsePHIInst &I);
  void visitDefPHIInst(DefPHIInst &I);
  void visitArgPHIInst(ArgPHIInst &I);
  void visitRetPHIInst(RetPHIInst &I);
  // Access operations
  void visitMutStructWriteInst(MutStructWriteInst &I);
  void visitMutIndexWriteInst(MutIndexWriteInst &I);
  void visitMutAssocWriteInst(MutAssocWriteInst &I);
  void visitIndexReadInst(IndexReadInst &I);
  void visitAssocReadInst(AssocReadInst &I);
  void visitIndexGetInst(IndexGetInst &I);
  void visitAssocGetInst(AssocGetInst &I);
  // MUT Sequence operations
  void visitMutSeqInsertInst(MutSeqInsertInst &I);
  void visitMutSeqInsertSeqInst(MutSeqInsertSeqInst &I);
  void visitMutSeqRemoveInst(MutSeqRemoveInst &I);
  void visitMutSeqAppendInst(MutSeqAppendInst &I);
  void visitMutSeqSwapInst(MutSeqSwapInst &I);
  void visitMutSeqSwapWithinInst(MutSeqSwapWithinInst &I);
  void visitMutSeqSplitInst(MutSeqSplitInst &I);
  // SSA Assoc operations
  void visitAssocHasInst(AssocHasInst &I);
  // MUT Assoc operations
  void visitMutAssocInsertInst(MutAssocInsertInst &I);
  void visitMutAssocRemoveInst(MutAssocRemoveInst &I);

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

} // namespace llvm::memoir
