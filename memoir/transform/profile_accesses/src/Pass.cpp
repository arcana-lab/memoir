// LLVM
#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"

// MEMOIR
#include "AccessCounter.hpp"
#include "memoir/passes/Passes.hpp"

namespace memoir {

/*
 * This pass injects profiling for sparse versus dense accesses.
 *
 * Author(s): Tommy McMichen
 * Created: April 14, 2025
 */

static llvm::cl::opt<bool> profile_accesses(
    "memoir-profile-accesses",
    llvm::cl::desc("Instrument the program to count dynamic accesses"));

llvm::PreservedAnalyses ProfileAccessesPass::run(
    llvm::Module &M,
    llvm::ModuleAnalysisManager &MAM) {

  bool modified = false;

  if (profile_accesses) {
    AccessCounter counters(M);

    modified = true;
  }

  return modified ? llvm::PreservedAnalyses::none()
                  : llvm::PreservedAnalyses::all();
}

} // namespace memoir
