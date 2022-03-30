#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include "noelle/core/Noelle.hpp"

#include "ObjectLowering.hpp"

using namespace llvm::noelle ;

namespace {

  struct ObjectLowering : public ModulePass {
    static char ID; 

    ObjectLowering() : ModulePass(ID) {}

    bool doInitialization (Module &M) override {
      return false;
    }

    bool runOnModule (Module &M) override {

      /*
       * Fetch NOELLE
       */
      auto& noelle = getAnalysis<Noelle>();

      /*
       * Use NOELLE
       */
      auto insts = noelle.numberOfProgramInstructions();
      errs() << "The program has " << insts << " instructions\n";

      /*
       * Find calls to object-ir API
       */
      for (auto &F : M) {
        for (auto &I : F) {
          if (auto callInst = dyn_cast<CallInst>(&I)) {
            auto callee = callInst->getCalledFunction();
            if (!callee) {
              continue;
            }
            
            auto calleeName = callee->getName();
            errs() << calleeName << "\n";
          }
        }
      }

      return false;
    }

    void getAnalysisUsage(AnalysisUsage &AU) const override {
      AU.addRequired<Noelle>();
    }
  };

}

// Next there is code to register your pass to "opt"
char ObjectLowering::ID = 0;
static RegisterPass<ObjectLowering> X("ObjectLowering", "Lowers the object-ir language to LLVM IR");

// Next there is code to register your pass to "clang"
static ObjectLowering * _PassMaker = NULL;
static RegisterStandardPasses _RegPass1(PassManagerBuilder::EP_OptimizerLast,
    [](const PassManagerBuilder&, legacy::PassManagerBase& PM) {
        if(!_PassMaker){ PM.add(_PassMaker = new ObjectLowering());}}); // ** for -Ox
static RegisterStandardPasses _RegPass2(PassManagerBuilder::EP_EnabledOnOptLevel0,
    [](const PassManagerBuilder&, legacy::PassManagerBase& PM) {
        if(!_PassMaker){ PM.add(_PassMaker = new ObjectLowering()); }}); // ** for -O0
