#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include "noelle/core/Noelle.hpp"

#include "ObjectAbstraction.hpp"

using namespace llvm::noelle;
using namespace object_abstraction;

namespace {

  struct ObjectAbstractionPass : public ModulePass {
    static char ID; 

    ObjectAbstractionPass() : ModulePass(ID) {}

    bool doInitialization (Module &M) override {
      return false;
    }

    bool runOnModule (Module &M) override {
            
      /*
       * Fetch NOELLE
       */
      auto &noelle = getAnalysis<Noelle>();
      auto pdg = noelle.getProgramDependenceGraph();

      auto ObjectAbstraction = new object_abstraction::ObjectAbstraction(M, &noelle, pdg);

      ObjectAbstraction->analyze();

      ObjectAbstraction->transform();
      
      return false;
    }

    void getAnalysisUsage(AnalysisUsage &AU) const override {
      AU.addRequired<Noelle>();
    }
  };

}

// Next there is code to register your pass to "opt"
char ObjectAbstractionPass::ID = 0;
static RegisterPass<ObjectAbstractionPass> X("ObjectAbstraction", "Pass to build object representations at object-IR level");

// Next there is code to register your pass to "clang"
static ObjectAbstractionPass * _PassMaker = NULL;
static RegisterStandardPasses _RegPass1(PassManagerBuilder::EP_OptimizerLast,
    [](const PassManagerBuilder&, legacy::PassManagerBase& PM) {
        if(!_PassMaker){ PM.add(_PassMaker = new ObjectAbstractionPass());}}); // ** for -Ox
static RegisterStandardPasses _RegPass2(PassManagerBuilder::EP_EnabledOnOptLevel0,
    [](const PassManagerBuilder&, legacy::PassManagerBase& PM) {
        if(!_PassMaker){ PM.add(_PassMaker = new ObjectAbstractionPass()); }}); // ** for -O0
