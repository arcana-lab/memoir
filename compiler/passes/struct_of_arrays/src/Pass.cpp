#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include "noelle/core/Noelle.hpp"

#include "StructOfArrays.hpp"

using namespace llvm::noelle;

namespace {

struct ObjectLoweringPass : public ModulePass {
  static char ID;

  ObjectLoweringPass() : ModulePass(ID) {}

  bool doInitialization(Module &M) override {
    return false;
  }

  bool runOnModule(Module &M) override {

    /*
     * Fetch NOELLE
     */
    auto &noelle = getAnalysis<Noelle>();

    auto structOfArrays =
        new struct_of_arrays::StructOfArrays(M, &noelle);

    structOfArrays->analyze();

    structOfArrays->transform();

    return false;
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<Noelle>();
  }
};

} // namespace

// Next there is code to register your pass to "opt"
char ObjectLoweringPass::ID = 0;
static RegisterPass<ObjectLoweringPass> X(
    "ObjectLoweringPass",
    "Lowers the object-ir language to LLVM IR");

// Next there is code to register your pass to "clang"
static ObjectLoweringPass *_PassMaker = NULL;
static RegisterStandardPasses _RegPass1(
    PassManagerBuilder::EP_OptimizerLast,
    [](const PassManagerBuilder &,
       legacy::PassManagerBase &PM) {
      if (!_PassMaker) {
        PM.add(_PassMaker = new ObjectLoweringPass());
      }
    }); // ** for -Ox
static RegisterStandardPasses _RegPass2(
    PassManagerBuilder::EP_EnabledOnOptLevel0,
    [](const PassManagerBuilder &,
       legacy::PassManagerBase &PM) {
      if (!_PassMaker) {
        PM.add(_PassMaker = new ObjectLoweringPass());
      }
    }); // ** for -O0
