#include <string>

// LLVM
#include "llvm/IR/Function.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"

// MemOIR
#include "memoir/passes/Passes.hpp"

#include "memoir/ir/InstVisitor.hpp"
#include "memoir/ir/Instructions.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/support/DataTypes.hpp"
#include "memoir/support/Print.hpp"

// Dead Field Elimination
#include "DeadFieldElimination.hpp"

/*
 * This pass eliminates trivially dead fields from a struct definition.
 *
 * Author(s): Tommy McMichen
 * Created: August 25, 2023
 */

namespace memoir {

llvm::PreservedAnalyses DeadFieldEliminationPass::run(
    llvm::Module &M,
    llvm::ModuleAnalysisManager &MAM) {
  debugln("Running dead field elimination pass");
  debugln();

  DeadFieldElimination DFE(M);

  return DFE.transformed ? llvm::PreservedAnalyses::none()
                         : llvm::PreservedAnalyses::all();
}

} // namespace memoir
