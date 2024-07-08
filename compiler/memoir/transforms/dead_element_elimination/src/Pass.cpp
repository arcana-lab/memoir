// LLVM
#include "llvm/IR/Function.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"

// MemOIR
#include "memoir/passes/Passes.hpp"

#include "memoir/ir/InstVisitor.hpp"
#include "memoir/ir/Instructions.hpp"

#include "memoir/analysis/LiveRangeAnalysis.hpp"

#include "memoir/support/Print.hpp"

#include "DeadElementElimination.hpp"

namespace llvm::memoir {

/*
 * This pass eliminates updates to dead elements of collections.
 *
 * Author(s): Tommy McMichen
 * Created: January 4, 2024
 */

llvm::PreservedAnalyses DeadElementEliminationPass::run(
    llvm::Module &M,
    llvm::ModuleAnalysisManager &MAM) {
  debugln("Running dead element elimination pass");
  debugln();

  auto &LRAR = MAM.getResult<LiveRangeAnalysis>(M);

  DeadElementElimination DEE(M, LRAR);

  return llvm::PreservedAnalyses::none();
}

} // namespace llvm::memoir
