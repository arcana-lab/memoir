#include <iostream>
#include <string>

// LLVM
#include "llvm/IR/Module.h"

// MemOIR
#include "memoir/passes/Passes.hpp"
#include "memoir/support/Print.hpp"
#include "memoir/transforms/utilities/ReifyTempArgs.hpp"

using namespace llvm::memoir;

/*
 * This pass erases all existing LiveOutMetadata and re-inserts it.
 *
 * Author(s): Tommy McMichen
 * Created: September 24, 2024
 */

namespace llvm::memoir {

llvm::PreservedAnalyses TempArgReificationPass::run(
    llvm::Module &M,
    llvm::ModuleAnalysisManager &MAM) {

  infoln();
  infoln("BEGIN Temporary Argument Reification pass");
  infoln();

  auto modified = reify_tempargs(M);

  return modified ? llvm::PreservedAnalyses::none()
                  : llvm::PreservedAnalyses::all();
}

} // namespace llvm::memoir
