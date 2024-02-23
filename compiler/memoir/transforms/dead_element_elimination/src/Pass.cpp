#include <iostream>
#include <string>

// LLVM
#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

// MemOIR
#include "memoir/ir/InstVisitor.hpp"
#include "memoir/ir/Instructions.hpp"

#include "memoir/analysis/LiveRangeAnalysis.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/support/InternalDatatypes.hpp"
#include "memoir/support/Print.hpp"

#include "DeadElementElimination.hpp"

namespace llvm::memoir {

/*
 * This pass eliminates updates to dead elements of collections.
 *
 * Author(s): Tommy McMichen
 * Created: January 4, 2024
 */

struct DeadElementEliminationPass : public ModulePass {
  static char ID;

  DeadElementEliminationPass() : ModulePass(ID) {}

  bool doInitialization(llvm::Module &M) override {
    return false;
  }

  bool runOnModule(llvm::Module &M) override {
    println("Running dead element elimination pass");
    println();

    auto &noelle = getAnalysis<arcana::noelle::Noelle>();

    LiveRangeAnalysis LRA(M, noelle);

    DeadElementElimination DEE(M, LRA);

    return true;
  }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.addRequired<arcana::noelle::Noelle>();
    return;
  }
};

// Next there is code to register your pass to "opt"
char DeadElementEliminationPass::ID = 0;
static llvm::RegisterPass<DeadElementEliminationPass> X(
    "memoir-dee",
    "Eliminates dead element updates.");

} // namespace llvm::memoir
