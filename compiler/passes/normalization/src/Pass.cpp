#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Metadata.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Support/CommandLine.h"

#include "noelle/core/Noelle.hpp"

#include "Normalization.hpp"

using namespace llvm::noelle;

namespace {

static cl::opt<bool> OnlyRuntime(
    "only-runtime",
    cl::init(false),
    cl::desc("Only target the runtime, inject metadata."));

struct NormalizationPass : public ModulePass {
  static char ID;

  NormalizationPass() : ModulePass(ID) {}

  bool doInitialization(Module &M) override {
    return false;
  }

  bool runOnModule(Module &M) override {
    errs() << "Running normalization pass\n";
    
    auto normalization = new normalization::Normalization(M);
    
    if (OnlyRuntime) {
      errs() << "Normalizing ObjectIR Runtime\n";
      normalization->transformRuntime();

      return true;
    }

    normalization->analyze();

    normalization->transform();

    return true;
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    return;
  }
};

} // namespace

// Next there is code to register your pass to "opt"
char NormalizationPass::ID = 0;
static RegisterPass<NormalizationPass> X(
    "NormalizationPass",
    "Normalizes the object-ir language and runtime");

// Next there is code to register your pass to "clang"
static NormalizationPass *_PassMaker = NULL;
static RegisterStandardPasses _RegPass1(PassManagerBuilder::EP_OptimizerLast,
                                        [](const PassManagerBuilder &,
                                           legacy::PassManagerBase &PM) {
                                          if (!_PassMaker) {
                                            PM.add(_PassMaker =
                                                       new NormalizationPass());
                                          }
                                        }); // ** for -Ox
static RegisterStandardPasses _RegPass2(
    PassManagerBuilder::EP_EnabledOnOptLevel0,
    [](const PassManagerBuilder &, legacy::PassManagerBase &PM) {
      if (!_PassMaker) {
        PM.add(_PassMaker = new NormalizationPass());
      }
    }); // ** for -O0
