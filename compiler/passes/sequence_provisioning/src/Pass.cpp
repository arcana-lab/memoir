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
#include "memoir/ir/Instructions.hpp"

#include "memoir/analysis/CollectionAnalysis.hpp"
#include "memoir/analysis/SizeAnalysis.hpp"
#include "memoir/analysis/StructAnalysis.hpp"
#include "memoir/analysis/TypeAnalysis.hpp"
#include "memoir/analysis/ValueNumbering.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/support/InternalDatatypes.hpp"
#include "memoir/support/Print.hpp"

#include "memoir/utility/FunctionNames.hpp"
#include "memoir/utility/Metadata.hpp"

// SequenceProvisioner
#include "SequenceProvisioner.hpp"

using namespace llvm::memoir;

/*
 * This pass analyzes the joins and slices of a sequence to provision the
 * required size of a sequence ahead of time.
 *
 * Author(s): Tommy McMichen
 * Created: February 22, 2023
 */

namespace llvm::memoir {

struct SequenceProvisioningPass : public ModulePass {
  static char ID;

  SequenceProvisioningPass() : ModulePass(ID) {}

  bool doInitialization(Module &M) override {
    return false;
  }

  bool runOnModule(Module &M) override {
    println("Running sequence provisioning pass");
    println();

    // Get the required analyses.
    auto &noelle = getAnalysis<llvm::noelle::Noelle>();
    auto &TA = TypeAnalysis::get();
    auto &SA = StructAnalysis::get();
    auto &CA = CollectionAnalysis::get(noelle);
    auto VN = ValueNumbering(M);
    auto SizeA = SizeAnalysis(noelle, VN);

    // Construct the SequenceProvisioner
    auto SP = SequenceProvisioner(M, noelle, CA, SizeA);
    SP.run();

    return false;
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<llvm::noelle::Noelle>();
    AU.addRequired<llvm::DominatorTreeWrapperPass>();
    return;
  }
};

} // namespace llvm::memoir

// Next there is code to register your pass to "opt"
char SequenceProvisioningPass::ID = 0;
static RegisterPass<SequenceProvisioningPass> X(
    "SequenceProvisioning",
    "This pass attempts to provision the needed size of a sequence ahead of time, removing the number of joins needed.");

// Next there is code to register your pass to "clang"
static SequenceProvisioningPass *_PassMaker = NULL;
static RegisterStandardPasses _RegPass1(
    PassManagerBuilder::EP_OptimizerLast,
    [](const PassManagerBuilder &, legacy::PassManagerBase &PM) {
      if (!_PassMaker) {
        PM.add(_PassMaker = new SequenceProvisioningPass());
      }
    }); // ** for -Ox
static RegisterStandardPasses _RegPass2(
    PassManagerBuilder::EP_EnabledOnOptLevel0,
    [](const PassManagerBuilder &, legacy::PassManagerBase &PM) {
      if (!_PassMaker) {
        PM.add(_PassMaker = new SequenceProvisioningPass());
      }
    }); // ** for -O0
