// LLVM
#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"

#include "llvm/Analysis/CallGraph.h"

// MEMOIR
#include "memoir/passes/Passes.hpp"

#include "memoir/raising/TypeInference.hpp"

namespace memoir {

/*
 * This pass performs type inference on MEMOIR variables, adding explicit type
 * annotations where necessary.
 *
 * Author(s): Tommy McMichen
 * Created: December 19, 2023
 */

llvm::PreservedAnalyses TypeInferencePass::run(
    llvm::Module &M,
    llvm::ModuleAnalysisManager &MAM) {

  auto type_inference = new TypeInference(M);

  auto modified = type_inference->run();

  return modified ? llvm::PreservedAnalyses::none()
                  : llvm::PreservedAnalyses::all();
}

} // namespace memoir
