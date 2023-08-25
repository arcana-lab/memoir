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

// Dead Field Elimination
#include "DeadFieldElimination.hpp"

using namespace llvm::memoir;

/*
 * This pass eliminates trivially dead fields from a struct definition.
 *
 * Author(s): Tommy McMichen
 * Created: August 25, 2023
 */

struct DeadFieldEliminationPass : public ModulePass {
  static char ID;

  DeadFieldEliminationPass() : ModulePass(ID) {}

  bool doInitialization(Module &M) override {
    return false;
  }

  bool runOnModule(Module &M) override {
    println("Running dead field elimination pass");
    println();

    auto DFE = DeadFieldElimination(M);

    return DFE.transformed;
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    return;
  }
};

// Next there is code to register your pass to "opt"
char DeadFieldEliminationPass::ID = 0;
static RegisterPass<DeadFieldEliminationPass> X(
    "memoir-dfe",
    "Eliminates dead fields from type definition.");

// Next there is code to register your pass to "clang"
static DeadFieldEliminationPass *_PassMaker = NULL;
static RegisterStandardPasses _RegPass1(
    PassManagerBuilder::EP_OptimizerLast,
    [](const PassManagerBuilder &, legacy::PassManagerBase &PM) {
      if (!_PassMaker) {
        PM.add(_PassMaker = new DeadFieldEliminationPass());
      }
    }); // ** for -Ox
static RegisterStandardPasses _RegPass2(
    PassManagerBuilder::EP_EnabledOnOptLevel0,
    [](const PassManagerBuilder &, legacy::PassManagerBase &PM) {
      if (!_PassMaker) {
        PM.add(_PassMaker = new DeadFieldEliminationPass());
      }
    }); // ** for -O0
