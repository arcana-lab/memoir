#include <iostream>
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
#include "memoir/support/InternalDatatypes.hpp"
#include "memoir/support/Print.hpp"

// Dead Code Elimination
#include "DeadCollectionElimination.hpp"

namespace llvm::memoir {

/*
 * This pass eliminates dead collections.
 *
 * Author(s): Tommy McMichen
 * Created: February 22, 2023
 */

llvm::PreservedAnalyses DeadCollectionEliminationPass::run(
    llvm::Module &M,
    llvm::ModuleAnalysisManager &MAM) {
  debugln("Running dead collection elimination pass");
  debugln();

  DeadCollectionElimination DCE(M);

  return llvm::PreservedAnalyses::none();
}

} // namespace llvm::memoir
