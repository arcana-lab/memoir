#include "folio/analysis/ConstraintInference.hpp"

#include "folio/passes/Pass.hpp"

namespace folio {

llvm::PreservedAnalyses FolioPass::run(llvm::Module &M,
                                       llvm::ModuleAnalysisManager &MAM) {

  // Fetch the ConstraintInference results.
  auto &constraints = MAM.getResult<ConstraintInference>(M);

  return llvm::PreservedAnalyses::none();
}

} // namespace folio
