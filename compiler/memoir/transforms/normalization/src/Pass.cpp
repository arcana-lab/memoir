#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include "noelle/core/Noelle.hpp"

#include "Normalization.hpp"

using namespace arcana::noelle;

namespace llvm::memoir {

static cl::opt<bool> OnlyRuntime("only-runtime",
                                 cl::init(false),
                                 cl::desc("Only target the runtime."));

struct NormalizationPass : public ModulePass {
  static char ID;

  NormalizationPass() : ModulePass(ID) {}

  bool doInitialization(Module &M) override {
    return false;
  }

  bool runOnModule(Module &M) override {
    errs() << "Running normalization pass\n";

    auto normalization = new Normalization(M);

    if (OnlyRuntime) {
      errs() << "Normalizing MemOIR Runtime\n";
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

} // namespace llvm::memoir

// Next there is code to register your pass to "opt"
char llvm::memoir::NormalizationPass::ID = 0;
static llvm::RegisterPass<llvm::memoir::NormalizationPass> X(
    "memoir-norm",
    "Normalizes the MemOIR language and runtime");
