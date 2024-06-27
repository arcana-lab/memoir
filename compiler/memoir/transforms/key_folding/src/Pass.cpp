// LLVM
#include "llvm/IR/Function.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"

// MemOIR
#include "memoir/passes/Passes.hpp"

#include "memoir/ir/InstVisitor.hpp"
#include "memoir/ir/Instructions.hpp"

#include "memoir/support/InternalDatatypes.hpp"
#include "memoir/support/Print.hpp"

// Key Folding
#include "KeyFolding.hpp"

namespace llvm::memoir {

/*
 * This pass folds the key-space of an assoc onto a sequence's index space when
 * possible.
 *
 * Author(s): Tommy McMichen
 * Created: August 28, 2023
 */

llvm::PreservedAnalyses KeyFoldingPass::run(llvm::Module &M,
                                            llvm::ModuleAnalysisManager &MAM) {
  debugln("Running key folding pass");
  debugln();

  KeyFolding KF(M);

  return KF.transformed ? llvm::PreservedAnalyses::none()
                        : llvm::PreservedAnalyses::all();
}

} // namespace llvm::memoir
