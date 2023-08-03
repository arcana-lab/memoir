#include "llvm/IR/CFG.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"

#include "noelle/core/DominatorSummary.hpp"

#include "memoir/ir/Builder.hpp"
#include "memoir/ir/InstVisitor.hpp"
#include "memoir/ir/Instructions.hpp"

#include "llvm/ADT/MapVector.h"

namespace llvm::memoir {

using ReachingDefMapTy = map<llvm::Value *, llvm::Value *>;

class MutToImmutVisitor
  : public llvm::memoir::InstVisitor<MutToImmutVisitor, void> {
  friend class llvm::memoir::InstVisitor<MutToImmutVisitor, void>;
  friend class llvm::InstVisitor<MutToImmutVisitor, void>;

public:
  MutToImmutVisitor(llvm::noelle::DomTreeSummary &DT,
                    ordered_set<llvm::Value *> memoir_names,
                    map<llvm::PHINode *, llvm::Value *> inserted_phis);

  void visitInstruction(llvm::Instruction &I);
  void visitPHINode(llvm::PHINode &I);
  void visitUsePHIInst(UsePHIInst &I);
  void visitDefPHIInst(DefPHIInst &I);
  void visitIndexWriteInst(IndexWriteInst &I);
  void visitAssocWriteInst(AssocWriteInst &I);
  void visitIndexReadInst(IndexReadInst &I);
  void visitAssocReadInst(AssocReadInst &I);
  void visitSeqInsertInst(SeqInsertInst &I);
  void visitSeqRemoveInst(SeqRemoveInst &I);
  void visitSeqAppendInst(SeqAppendInst &I);
  void visitSeqSwapInst(SeqSwapInst &I);
  void visitSeqSplitInst(SeqSplitInst &I);

  llvm::Value *update_reaching_definition(llvm::Value *variable,
                                          MemOIRInst &program_point);
  llvm::Value *update_reaching_definition(llvm::Value *variable,
                                          llvm::Instruction &program_point);
  llvm::Value *update_reaching_definition(llvm::Value *variable,
                                          llvm::Instruction *program_point);

  void cleanup();

protected:
  MemOIRBuilder *builder;
  ReachingDefMapTy reaching_definitions;
  llvm::noelle::DomTreeSummary &DT;
  set<llvm::Instruction *> instructions_to_delete;
  map<llvm::PHINode *, llvm::Value *> inserted_phis;
};

} // namespace llvm::memoir
