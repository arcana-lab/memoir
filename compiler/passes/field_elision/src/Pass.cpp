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

#include "llvm/Analysis/CallGraph.h"

// MemOIR
#include "memoir/ir/InstVisitor.hpp"
#include "memoir/ir/Instructions.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/support/InternalDatatypes.hpp"
#include "memoir/support/Print.hpp"

// Field Elision
#include "FieldElision.hpp"

using namespace llvm::memoir;

/*
 * This pass performs the field elision optimization.
 *
 * Author(s): Tommy McMichen
 * Created: August 25, 2023
 */

struct FieldElisionPass : public ModulePass {
  static char ID;

  FieldElisionPass() : ModulePass(ID) {}

  bool doInitialization(Module &M) override {
    return false;
  }

  bool runOnModule(Module &M) override {
    println("========================");
    println("BEGIN field elision pass");
    println();

    // TODO: when we update the escape analysis to use the complete call graph,
    // also update users of this call graph to do the same..
    auto &CG = getAnalysis<llvm::CallGraphWrapperPass>().getCallGraph();

    FieldElision::FieldsToElideMapTy fields_to_elide = {};
    auto FE = FieldElision(M, CG, fields_to_elide);

    println();
    println("END field elision pass");
    println("========================");

    return FE.transformed;
    return false;
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<llvm::CallGraphWrapperPass>();
    return;
  }
};

// Next there is code to register your pass to "opt"
char FieldElisionPass::ID = 0;
static RegisterPass<FieldElisionPass> X(
    "memoir-fe",
    "Converts fields into associative arrays.");

// Next there is code to register your pass to "clang"
static FieldElisionPass *_PassMaker = NULL;
static RegisterStandardPasses _RegPass1(PassManagerBuilder::EP_OptimizerLast,
                                        [](const PassManagerBuilder &,
                                           legacy::PassManagerBase &PM) {
                                          if (!_PassMaker) {
                                            PM.add(_PassMaker =
                                                       new FieldElisionPass());
                                          }
                                        }); // ** for -Ox
static RegisterStandardPasses _RegPass2(
    PassManagerBuilder::EP_EnabledOnOptLevel0,
    [](const PassManagerBuilder &, legacy::PassManagerBase &PM) {
      if (!_PassMaker) {
        PM.add(_PassMaker = new FieldElisionPass());
      }
    }); // ** for -O0
