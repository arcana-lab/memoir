#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include "noelle/core/Noelle.hpp"
#include "ObjectAbstraction.hpp"

namespace object_abstraction {

  class ObjectAbstractionPass : public ModulePass {
  public:
    static char ID;

    ObjectAbstractionPass () : ModulePass(ID) {}

    bool doInitialization (Module &M) override { 
      return false;
    }

    bool runOnModule (Module &M) override {
      auto &noelle = getAnalysis<Noelle>();

      this->objectAbstraction = new object_abstraction::ObjectAbstraction(M, &noelle);
      this->objectAbstraction->contructObjectAbstractions();

      return false;
    }

    void getAnalysisUsage (AnalysisUsage &AU) const override {
      AU.addRequired<Noelle>();
    }

    /*
     * Fetch constructed ObjectAbstraction object
     */
    ObjectAbstraction * getObjectAbstraction () {
      return this->objectAbstraction;
    }

  private:
    ObjectAbstraction *objectAbstraction;
  };

// Next there is code to register your pass to "opt"
char ObjectAbstractionPass::ID = 0;
static RegisterPass<ObjectAbstractionPass> X("ObjectAbstraction", "Analysis pass to build object representations at LLVM-IR level");

// Next there is code to register your pass to "clang"
static ObjectAbstractionPass * _PassMaker = NULL;
static RegisterStandardPasses _RegPass1(PassManagerBuilder::EP_OptimizerLast,
    [](const PassManagerBuilder&, legacy::PassManagerBase& PM) {
        if(!_PassMaker){ PM.add(_PassMaker = new ObjectAbstractionPass());}}); // ** for -Ox
static RegisterStandardPasses _RegPass2(PassManagerBuilder::EP_EnabledOnOptLevel0,
    [](const PassManagerBuilder&, legacy::PassManagerBase& PM) {
        if(!_PassMaker){ PM.add(_PassMaker = new ObjectAbstractionPass()); }}); // ** for -O0

} // namespace object_abstraction