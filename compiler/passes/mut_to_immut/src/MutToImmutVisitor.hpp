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

struct MutToImmutStats {
  using CountTy = uint32_t;

  CountTy num_mut_collections = 0;
  CountTy num_ssa_collections = 0;
  CountTy num_trivial_ssa_collections = 0;

  void inc_mut(CountTy inc = 1) {
    this->num_mut_collections += inc;
  }

  void inc_ssa(CountTy inc = 1) {
    this->num_ssa_collections += inc;
  }

  void inc_trivial_ssa(CountTy inc = 1) {
    this->num_ssa_collections += inc;
    this->num_trivial_ssa_collections += inc;
  }
};

class MutToImmutVisitor
  : public llvm::memoir::InstVisitor<MutToImmutVisitor, void> {
  friend class llvm::memoir::InstVisitor<MutToImmutVisitor, void>;
  friend class llvm::InstVisitor<MutToImmutVisitor, void>;

public:
  MutToImmutVisitor(llvm::noelle::DomTreeSummary &DT,
                    ordered_set<llvm::Value *> memoir_names,
                    map<llvm::PHINode *, llvm::Value *> inserted_phis,
                    MutToImmutStats *stats = nullptr);

  // LLVM operations
  void visitInstruction(llvm::Instruction &I);
  void visitPHINode(llvm::PHINode &I);
  // SSA operations
  void visitUsePHIInst(UsePHIInst &I);
  void visitDefPHIInst(DefPHIInst &I);
  // Access operations
  void visitIndexWriteInst(IndexWriteInst &I);
  void visitAssocWriteInst(AssocWriteInst &I);
  void visitIndexReadInst(IndexReadInst &I);
  void visitAssocReadInst(AssocReadInst &I);
  void visitIndexGetInst(IndexGetInst &I);
  void visitAssocGetInst(AssocGetInst &I);
  // Sequence operations
  void visitSeqInsertInst(SeqInsertInst &I);
  void visitSeqRemoveInst(SeqRemoveInst &I);
  void visitSeqAppendInst(SeqAppendInst &I);
  void visitSeqSwapInst(SeqSwapInst &I);
  void visitSeqSplitInst(SeqSplitInst &I);
  // Assoc operations
  void visitAssocHasInst(AssocHasInst &I);
  void visitAssocRemoveInst(AssocRemoveInst &I);

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

  // Statistics
  void increment_ssa(uint64_t inc = 1);
  void increment_trivial_ssa(uint64_t inc = 1);
  MutToImmutStats *stats;
};

} // namespace llvm::memoir
