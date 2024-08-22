// LLVM
#include "llvm/IR/Module.h"

// MEMOIR
#include "memoir/passes/Passes.hpp"

#include "memoir/support/Print.hpp"

#include "LowerFold.hpp"

namespace llvm::memoir {

llvm::PreservedAnalyses LowerFoldPass::run(llvm::Module &M,
                                           llvm::ModuleAnalysisManager &MAM) {

  LowerFold LF(M);

  MemOIRInst::invalidate();

  return llvm::PreservedAnalyses::none();
}

} // namespace llvm::memoir
