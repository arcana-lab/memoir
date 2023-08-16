#include <iostream>
#include <string>

// LLVM
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"

#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

// MemOIR
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

// Instantiate the command line options we need.
Verbosity VerboseLevel;
static llvm::cl::opt<llvm::memoir::Verbosity, true> VerboseLevelOpt(
    "memoir-verbose",
    llvm::cl::desc("Set the verbosity"),
    llvm::cl::values(
        clEnumValN(noverbosity, "none", "disable verbose messages"),
        clEnumVal(quick, "only enable short-form messages"),
        clEnumVal(detailed, "enable all verbose messages")),
    cl::location(VerboseLevel));

// This pass exists because of annoying shared object linking.
struct CommandLinePass : public ModulePass {
  static char ID;

  CommandLinePass() : ModulePass(ID) {}

  bool doInitialization(Module &M) override {
    return false;
  }

  bool runOnModule(Module &M) override {
    println("print enabled");
    infoln("quick enabled");
    debugln("debug enabled");

    return false;
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    return;
  }
};

} // namespace llvm::memoir

// Next there is code to register your pass to "opt"
char memoir::CommandLinePass::ID = 0;
static RegisterPass<memoir::CommandLinePass> X(
    "memoir-cl",
    "Gathers common command line options.");

// Next there is code to register your pass to "clang"
static memoir::CommandLinePass *_PassMaker = NULL;
static RegisterStandardPasses _RegPass1(
    PassManagerBuilder::EP_OptimizerLast,
    [](const PassManagerBuilder &, legacy::PassManagerBase &PM) {
      if (!_PassMaker) {
        PM.add(_PassMaker = new memoir::CommandLinePass());
      }
    }); // ** for -Ox
static RegisterStandardPasses _RegPass2(
    PassManagerBuilder::EP_EnabledOnOptLevel0,
    [](const PassManagerBuilder &, legacy::PassManagerBase &PM) {
      if (!_PassMaker) {
        PM.add(_PassMaker = new memoir::CommandLinePass());
      }
    }); // ** for -O0
