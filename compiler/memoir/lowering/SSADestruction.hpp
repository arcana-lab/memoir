#include "llvm/IR/CFG.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"

#include "memoir/ir/Builder.hpp"
#include "memoir/ir/InstVisitor.hpp"
#include "memoir/ir/Instructions.hpp"

#include "memoir/lowering/TypeLayout.hpp"

#include "llvm/ADT/MapVector.h"

namespace memoir {

using ReachingDefMapTy = Map<llvm::Value *, llvm::Value *>;

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
  : public memoir::InstVisitor<SSADestructionVisitor, void> {
  friend class memoir::InstVisitor<SSADestructionVisitor, void>;
  friend class llvm::InstVisitor<SSADestructionVisitor, void>;

public:
  SSADestructionVisitor(llvm::Module &M, SSADestructionStats *stats = nullptr);

  void setAnalyses(llvm::DominatorTree &DT);

  // LLVM operations
  void visitInstruction(llvm::Instruction &I);

  // Allocation operations
  void visitAllocInst(AllocInst &I);

  // Deallocation operationts
  void visitDeleteInst(DeleteInst &I);

  // Access operations
  void visitReadInst(ReadInst &I);
  void visitGetInst(GetInst &I);
  void visitCopyInst(CopyInst &I);
  void visitKeysInst(KeysInst &I);
  void visitSizeInst(SizeInst &I);
  void visitFoldInst(FoldInst &I);
  void visitClearInst(ClearInst &I);
  void visitHasInst(HasInst &I);

  // Update operations
  void visitWriteInst(WriteInst &I);
  void visitInsertInst(InsertInst &I);
  void visitRemoveInst(RemoveInst &I);

  // SSA collection operations
  void visitUsePHIInst(UsePHIInst &I);
  void visitRetPHIInst(RetPHIInst &I);

  // Other operations
  void visitEndInst(EndInst &I);

  // Typechecking
  void visitTypeInst(TypeInst &I);
  void visitAssertTypeInst(AssertTypeInst &I);
  void visitReturnTypeInst(ReturnTypeInst &I);

  // Staged lowering
  void stage(llvm::Instruction &I);
  void clear_stage();
  const Set<llvm::Instruction *> &staged();

  void do_coalesce(llvm::Value &V);

  void cleanup();

protected:
  // Analyses.
  llvm::Module &M;
  llvm::DominatorTree *DT;
  TypeConverter TC;

  // Owned state.
  Map<MemOIRInst *, detail::View *> inst_to_view;

  // Borrowed state.
  Map<llvm::Value *, llvm::Value *> coalesced_values;
  Map<llvm::Value *, llvm::Value *> replaced_values;
  Map<llvm::Value *, llvm::Value *> def_phi_replacements;
  OrderedSet<llvm::Instruction *> instructions_to_delete;

  Set<llvm::Instruction *> _staged;

  llvm::Value *find_replacement(llvm::Value *value);

  void coalesce(MemOIRInst &I, llvm::Value &replacement);
  void coalesce(llvm::Value &V, llvm::Value &replacement);

  void markForCleanup(MemOIRInst &I);
  void markForCleanup(llvm::Instruction &I);

  // Statistics
  SSADestructionStats *stats;
};

} // namespace memoir
