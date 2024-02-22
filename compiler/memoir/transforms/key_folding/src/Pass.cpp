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

// Key Folding
#include "KeyFolding.hpp"

namespace llvm::memoir {

/*
 * This pass folds the key-space of an assoc onto a sequence's index space when
 * possible.
 *
 * Author(s): Tommy McMichen
 * Created: August 28, 2023
 */

struct KeyFoldingPass : public ModulePass {
  static char ID;

  KeyFoldingPass() : ModulePass(ID) {}

  bool doInitialization(Module &M) override {
    return false;
  }

  bool runOnModule(Module &M) override {
    println("Running key folding pass");
    println();

    auto KF = KeyFolding(M);

    return KF.transformed;
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    return;
  }
};

// Next there is code to register your pass to "opt"
char KeyFoldingPass::ID = 0;
static llvm::RegisterPass<KeyFoldingPass> X(
    "memoir-kf",
    "Folds the key-space of an assoc onto a sequence when possible.");

} // namespace llvm::memoir
