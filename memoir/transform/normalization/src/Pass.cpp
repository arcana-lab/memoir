// LLVM
#include "llvm/Pass.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

// MEMOIR
#include "memoir/passes/Passes.hpp"

#include "memoir/support/Print.hpp"

#include "Normalization.hpp"

namespace memoir {

static llvm::cl::opt<bool> OnlyRuntime(
    "only-runtime",
    llvm::cl::init(false),
    llvm::cl::desc("Only target the runtime."));

llvm::PreservedAnalyses NormalizationPass::run(
    llvm::Module &M,
    llvm::ModuleAnalysisManager &MAM) {
  infoln("Running normalization pass\n");

  auto normalization = new Normalization(M);

  if (OnlyRuntime) {
    infoln("Normalizing MemOIR Runtime\n");
    normalization->transformRuntime();

    return llvm::PreservedAnalyses::none();
  }

  normalization->analyze();

  normalization->transform();

  return llvm::PreservedAnalyses::none();
}

} // namespace memoir
