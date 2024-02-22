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

#include "memoir/support/Assert.hpp"
#include "memoir/support/InternalDatatypes.hpp"
#include "memoir/support/Print.hpp"

// Dead Code Elimination
#include "DeadCollectionElimination.hpp"

namespace llvm::memoir {

/*
 * This pass canonicalizes the conservative iteration space of sequences in
 * loops by inserting slice and join operations before and after the loop.
 *
 * Author(s): Tommy McMichen
 * Created: February 22, 2023
 */

struct DeadCollectionEliminationPass : public ModulePass {
  static char ID;

  DeadCollectionEliminationPass() : ModulePass(ID) {}

  bool doInitialization(Module &M) override {
    return false;
  }

  bool runOnModule(Module &M) override {
    println("Running dead collection elimination pass");
    println();

    auto DCE = DeadCollectionElimination(M);

    return true;
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    return;
  }
};

// Next there is code to register your pass to "opt"
char DeadCollectionEliminationPass::ID = 0;
static llvm::RegisterPass<DeadCollectionEliminationPass> X(
    "memoir-dce",
    "Eliminates dead collection allocations.");

} // namespace llvm::memoir
