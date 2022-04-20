#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include "noelle/core/Noelle.hpp"

#include "ObjectInlining.hpp"

using namespace llvm::noelle;
using namespace object_inlining;

namespace {

  struct ObjectInliningPass : public ModulePass {
    static char ID; 

    ObjectInliningPass() : ModulePass(ID) {}

    bool doInitialization (Module &M) override {
      return false;
    }

    bool runOnModule (Module &M) override {
            
      /*
       * Fetch NOELLE
       */
      auto &noelle = getAnalysis<Noelle>();

      auto ObjectInlining = new object_inlining::ObjectInlining(M, &noelle);

      ObjectInlining->analyze();

      ObjectInlining->transform();
      
      return false;
    }

    void getAnalysisUsage(AnalysisUsage &AU) const override {
      AU.addRequired<Noelle>();
    }
  };

}

// Next there is code to register your pass to "opt"
char ObjectInliningPass::ID = 0;
static RegisterPass<ObjectInliningPass> X("ObjectInlining", "Pass to perform object inlining optimization at object-IR level");

// Next there is code to register your pass to "clang"
static ObjectInliningPass * _PassMaker = NULL;
static RegisterStandardPasses _RegPass1(PassManagerBuilder::EP_OptimizerLast,
    [](const PassManagerBuilder&, legacy::PassManagerBase& PM) {
        if(!_PassMaker){ PM.add(_PassMaker = new ObjectInliningPass());}}); // ** for -Ox
static RegisterStandardPasses _RegPass2(PassManagerBuilder::EP_EnabledOnOptLevel0,
    [](const PassManagerBuilder&, legacy::PassManagerBase& PM) {
        if(!_PassMaker){ PM.add(_PassMaker = new ObjectInliningPass()); }}); // ** for -O0
