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

struct SSADestructionStats {
  using CountTy = uint32_t;
};

class SSADestructionVisitor
  : public llvm::memoir::InstVisitor<SSADestructionVisitor, void> {
  friend class llvm::memoir::InstVisitor<SSADestructionVisitor, void>;
  friend class llvm::InstVisitor<SSADestructionVisitor, void>;

public:
  SSADestructionVisitor(llvm::noelle::DomTreeSummary &DT,
                        SSADestructionStats *stats = nullptr);

  // LLVM operations
  void visitInstruction(llvm::Instruction &I);

  // SSA operations
  void visitUsePHIInst(UsePHIInst &I);
  void visitDefPHIInst(DefPHIInst &I);

  void cleanup();

protected:
  llvm::noelle::DomTreeSummary &DT;
  set<llvm::Instruction *> instructions_to_delete;

  void markForCleanup(MemOIRInst &I);
  void markForCleanup(llvm::Instruction &I);

  // Statistics
  SSADestructionStats *stats;
};

} // namespace llvm::memoir
