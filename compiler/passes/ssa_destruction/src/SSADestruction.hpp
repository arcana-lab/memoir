#include "llvm/IR/CFG.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"

#include "memoir/ir/Builder.hpp"
#include "memoir/ir/InstVisitor.hpp"
#include "memoir/ir/Instructions.hpp"

#include "memoir/analysis/LivenessAnalysis.hpp"
#include "memoir/analysis/ValueNumbering.hpp"

#include "memoir/lowering/TypeLayout.hpp"

#include "llvm/ADT/MapVector.h"

namespace llvm::memoir {

using ReachingDefMapTy = map<llvm::Value *, llvm::Value *>;

struct SSADestructionStats {
  using CountTy = uint32_t;
};

namespace detail {
struct View {
public:
  View(llvm::Value &base, llvm::Value &begin, llvm::Value &end)
    : base(base),
      begin(begin),
      end(end) {}

  llvm::Value &get_base() const {
    return this->base;
  }

  llvm::Value &get_begin() const {
    return this->begin;
  }

  llvm::Value &get_end() const {
    return this->end;
  }

protected:
  llvm::Value &base;
  llvm::Value &begin;
  llvm::Value &end;
};
} // namespace detail

class SSADestructionVisitor
  : public llvm::memoir::InstVisitor<SSADestructionVisitor, void> {
  friend class llvm::memoir::InstVisitor<SSADestructionVisitor, void>;
  friend class llvm::InstVisitor<SSADestructionVisitor, void>;

public:
  SSADestructionVisitor(llvm::Module &M, SSADestructionStats *stats = nullptr);

  void setAnalyses(llvm::noelle::DominatorForest &DT,
                   LivenessAnalysis &LA,
                   ValueNumbering &VN);

  // LLVM operations
  void visitInstruction(llvm::Instruction &I);

  // Allocation operations
  void visitSequenceAllocInst(SequenceAllocInst &I);
  void visitAssocArrayAllocInst(AssocArrayAllocInst &I);
  void visitStructAllocInst(StructAllocInst &I);

  // Deallocation operationts
  void visitDeleteCollectionInst(DeleteCollectionInst &I);

  // Access operations
  void visitIndexReadInst(IndexReadInst &I);
  void visitIndexWriteInst(IndexWriteInst &I);
  void visitIndexGetInst(IndexGetInst &I);
  void visitAssocReadInst(AssocReadInst &I);
  void visitAssocWriteInst(AssocWriteInst &I);
  void visitAssocGetInst(AssocGetInst &I);
  void visitAssocHasInst(AssocHasInst &I);
  void visitAssocRemoveInst(AssocRemoveInst &I);
  void visitStructReadInst(StructReadInst &I);
  void visitStructWriteInst(StructWriteInst &I);
  void visitStructGetInst(StructGetInst &I);

  // SSA operations
  void visitUsePHIInst(UsePHIInst &I);
  void visitDefPHIInst(DefPHIInst &I);
  void visitSliceInst(SliceInst &I);
  void visitJoinInst(JoinInst &I);
  void visitSizeInst(SizeInst &I);

  // Typechecking
  void visitAssertCollectionTypeInst(AssertCollectionTypeInst &I);
  void visitAssertStructTypeInst(AssertStructTypeInst &I);
  void visitReturnTypeInst(ReturnTypeInst &I);

  void do_coalesce(llvm::Value &V);

  void cleanup();

protected:
  // Analyses.
  llvm::Module &M;
  llvm::noelle::DominatorForest *DT;
  LivenessAnalysis *LA;
  ValueNumbering *VN;
  TypeConverter TC;

  // Owned state.
  map<MemOIRInst *, detail::View *> inst_to_view;

  // Borrowed state.
  map<llvm::Value *, llvm::Value *> coalesced_values;
  map<llvm::Value *, llvm::Value *> replaced_values;
  map<llvm::Value *, llvm::Value *> def_phi_replacements;
  set<llvm::Instruction *> instructions_to_delete;

  llvm::Value *find_replacement(llvm::Value *value);

  void coalesce(MemOIRInst &I, llvm::Value &replacement);
  void coalesce(llvm::Value &V, llvm::Value &replacement);

  void markForCleanup(MemOIRInst &I);
  void markForCleanup(llvm::Instruction &I);

  // Statistics
  SSADestructionStats *stats;
};

} // namespace llvm::memoir
