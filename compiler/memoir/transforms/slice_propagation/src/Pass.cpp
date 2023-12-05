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

#include "llvm/IR/Dominators.h"

#include "llvm/Analysis/CFG.h"

// NOELLE
#include "noelle/core/Noelle.hpp"

// MemOIR
#include "memoir/ir/Builder.hpp"
#include "memoir/ir/Function.hpp"
#include "memoir/ir/InstVisitor.hpp"
#include "memoir/ir/Instructions.hpp"

#include "memoir/analysis/CollectionAnalysis.hpp"
#include "memoir/analysis/StructAnalysis.hpp"
#include "memoir/analysis/TypeAnalysis.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/support/InternalDatatypes.hpp"
#include "memoir/support/Print.hpp"

#include "memoir/utility/FunctionNames.hpp"
#include "memoir/utility/Metadata.hpp"

// Slice Propagation
#include "SlicePropagation.hpp"

using namespace llvm::memoir;

/*
 * This pass canonicalizes the conservative iteration space of sequences in
 * loops by inserting slice and join operations before and after the loop.
 *
 * Author(s): Tommy McMichen
 * Created: February 22, 2023
 */

namespace llvm::memoir {

struct SlicePropagationPass : public ModulePass {
  static char ID;

  SlicePropagationPass() : ModulePass(ID) {}

  bool doInitialization(Module &M) override {
    return false;
  }

  bool runOnModule(Module &M) override {
    println("Running slice propagation pass");
    println();

    // Get the required analyses.
    auto &noelle = getAnalysis<arcana::noelle::Noelle>();
    auto &TA = TypeAnalysis::get();
    auto &SA = StructAnalysis::get();
    auto &CA = CollectionAnalysis::get(noelle);

    auto SP = SlicePropagation(M, *this, noelle);

    auto analysis_succeeded = SP.analyze();
    if (!analysis_succeeded) {
      println("Slice Propagation analysis failed for some reason!");
      println("Not continuing with transformation.");
      return false;
    }

    auto transformed = SP.transform();

    return transformed;
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<arcana::noelle::Noelle>();
    AU.addRequired<llvm::DominatorTreeWrapperPass>();
    return;
  }
}; // namespace llvm::memoir

} // namespace llvm::memoir

// Next there is code to register your pass to "opt"
char SlicePropagationPass::ID = 0;
static RegisterPass<SlicePropagationPass> X(
    "SlicePropagation",
    "Propagates slices of collections used with loops of the program.");

// Next there is code to register your pass to "clang"
static SlicePropagationPass *_PassMaker = NULL;
static RegisterStandardPasses _RegPass1(
    PassManagerBuilder::EP_OptimizerLast,
    [](const PassManagerBuilder &, legacy::PassManagerBase &PM) {
      if (!_PassMaker) {
        PM.add(_PassMaker = new SlicePropagationPass());
      }
    }); // ** for -Ox
static RegisterStandardPasses _RegPass2(
    PassManagerBuilder::EP_EnabledOnOptLevel0,
    [](const PassManagerBuilder &, legacy::PassManagerBase &PM) {
      if (!_PassMaker) {
        PM.add(_PassMaker = new SlicePropagationPass());
      }
    }); // ** for -O0
