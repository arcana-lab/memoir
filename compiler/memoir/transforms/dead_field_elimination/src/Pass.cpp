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

namespace llvm::memoir {

struct DeadFieldEliminationPass : public ModulePass {
  static char ID;

  DeadFieldEliminationPass() : ModulePass(ID) {}

  bool doInitialization(llvm::Module &M) override {
    return false;
  }

  bool runOnModule(llvm::Module &M) override {
    debugln("Running dead field elimination pass");
    debugln();

    auto DFE = DeadFieldElimination(M);

    return DFE.transformed;
  }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    return;
  }
};
} // namespace llvm::memoir

// Next there is code to register your pass to "opt"
char llvm::memoir::DeadFieldEliminationPass::ID = 0;
static llvm::RegisterPass<llvm::memoir::DeadFieldEliminationPass> X(
    "memoir-dfe",
    "Eliminates dead fields from type definition.");
