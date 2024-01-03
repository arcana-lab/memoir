#include <iostream>
#include <string>

// LLVM
#include "llvm/Pass.h"

#include "llvm/IR/CFG.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"

#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

// MemOIR
#include "memoir/ir/InstVisitor.hpp"
#include "memoir/ir/Instructions.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/support/InternalDatatypes.hpp"
#include "memoir/support/Print.hpp"

#include "memoir/utility/FunctionNames.hpp"
#include "memoir/utility/Metadata.hpp"

using namespace llvm;

/*
 * This pass collects various statistics about a MemOIR program.
 *
 * Author(s): Tommy McMichen
 * Created: August 14, 2023
 */

namespace llvm::memoir {

struct MemOIRStats {
  using CountTy = uint32_t;

  MemOIRStats()
    : num_mut_collections(0),
      num_ssa_collections(0),
      num_trivial_ssa_collections(0) {}

  CountTy num_mut_collections;
  CountTy num_ssa_collections;
  CountTy num_trivial_ssa_collections;

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

class StatsVisitor : public llvm::memoir::InstVisitor<StatsVisitor> {
  friend class llvm::memoir::InstVisitor<StatsVisitor>;
  friend class llvm::InstVisitor<StatsVisitor>;

  MemOIRStats &stats;

public:
  StatsVisitor(MemOIRStats &stats) : stats(stats) {}

  void visitInstruction(llvm::Instruction &I) {
    // Do nothing.
  }

  void visitSequenceAllocInst(SequenceAllocInst &I) {
    stats.inc_mut();
    stats.inc_ssa();
  }

  void visitAssocArrayAllocInst(AssocArrayAllocInst &I) {
    stats.inc_mut();
    stats.inc_ssa();
  }

  void visitTensorAllocInst(TensorAllocInst &I) {
    stats.inc_mut();
    stats.inc_ssa();
  }

  void visitUsePHIInst(UsePHIInst &I) {
    stats.inc_trivial_ssa();
  }

  void visitDefPHIInst(DefPHIInst &I) {
    stats.inc_trivial_ssa();
  }

  void visitArgPHIInst(ArgPHIInst &I) {
    stats.inc_trivial_ssa();
  }

  void visitRetPHIInst(RetPHIInst &I) {
    stats.inc_trivial_ssa();
  }

  void visitIndexWriteInst(IndexWriteInst &I) {
    stats.inc_trivial_ssa();
  }

  void visitAssocWriteInst(AssocWriteInst &I) {
    stats.inc_trivial_ssa();
  }

  void visitInsertInst(InsertInst &I) {
    stats.inc_ssa();
  }

  void visitRemoveInst(RemoveInst &I) {
    stats.inc_ssa();
  }

  void visitCopyInst(CopyInst &I) {
    stats.inc_mut();
  }

  void visitSeqSwapInst(SeqSwapInst &I) {
    stats.inc_ssa(2);
  }

  void visitSeqSwapWithin(SeqSwapWithinInst &I) {
    stats.inc_ssa();
  }

  void visitAssocKeysInst(AssocKeysInst &I) {
    stats.inc_mut();
    stats.inc_ssa();
  }

  void visitMutSeqSplitInst(MutSeqSplitInst &I) {
    stats.inc_mut();
    stats.inc_ssa();
  }
};

struct MemOIRStatsPass : public ModulePass {
  static char ID;

  MemOIRStatsPass() : ModulePass(ID) {}

  bool doInitialization(Module &M) override {
    return false;
  }

  bool runOnModule(Module &M) override {
    println("BEGIN stats pass");
    println();

    MemOIRStats stats;
    StatsVisitor visitor(stats);

    for (auto &F : M) {
      if (F.empty()) {
        continue;
      }
      for (auto &BB : F) {
        for (auto &I : BB) {
          visitor.visit(I);
        }
      }
    }

    println("=========================");
    println("STATS");
    println("  NumMut = ", stats.num_mut_collections);
    println("  NumSSA = ", stats.num_ssa_collections);
    println("  NumTrivial = ", stats.num_trivial_ssa_collections);
    println("  NumNonTrivial = ",
            stats.num_ssa_collections - stats.num_trivial_ssa_collections);
    println("=========================");
    println("DONE stats pass");

    return false;
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    return;
  }
};

} // namespace llvm::memoir

// Next there is code to register your pass to "opt"
char memoir::MemOIRStatsPass::ID = 0;
static RegisterPass<memoir::MemOIRStatsPass> X(
    "memoir-stats",
    "Collects various statistics about a MemOIR program.");

// Next there is code to register your pass to "clang"
static memoir::MemOIRStatsPass *_PassMaker = NULL;
static RegisterStandardPasses _RegPass1(
    PassManagerBuilder::EP_OptimizerLast,
    [](const PassManagerBuilder &, legacy::PassManagerBase &PM) {
      if (!_PassMaker) {
        PM.add(_PassMaker = new memoir::MemOIRStatsPass());
      }
    }); // ** for -Ox
static RegisterStandardPasses _RegPass2(
    PassManagerBuilder::EP_EnabledOnOptLevel0,
    [](const PassManagerBuilder &, legacy::PassManagerBase &PM) {
      if (!_PassMaker) {
        PM.add(_PassMaker = new memoir::MemOIRStatsPass());
      }
    }); // ** for -O0
