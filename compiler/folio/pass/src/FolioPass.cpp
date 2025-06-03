#include "memoir/utility/Metadata.hpp"

#include "memoir/analysis/BoundsCheckAnalysis.hpp"
#include "memoir/support/Casting.hpp"
#include "memoir/support/DataTypes.hpp"
#include "memoir/support/PassUtils.hpp"
#include "memoir/support/Print.hpp"

#include "folio/transforms/LambdaLifting.hpp"
#include "folio/transforms/ProxyInsertion.hpp"
#include "folio/transforms/SelectionMonomorphization.hpp"

#if 0
#  include "folio/analysis/ConstraintInference.hpp"
#  include "folio/analysis/ContentAnalysis.hpp"
#  include "folio/opportunities/Analysis.hpp"

#  include "folio/solver/Implementation.hpp"
#  include "folio/solver/Solver.hpp"
#endif

#include "folio/pass/Pass.hpp"

using namespace llvm::memoir;

namespace folio {

llvm::PreservedAnalyses FolioPass::run(llvm::Module &M,
                                       llvm::ModuleAnalysisManager &MAM) {

  // First, we will normalize the code such that memoir functions are called at
  // most once.
  // LambdaLifting lifter(M);

  // Perform selection monomorphization.
  // { SelectionMonomorphization monomorph(M); }

  // Insert proxies and encode uses.
  {
    ProxyInsertion::GetDominatorTree get_dominator_tree =
        [&](llvm::Function &F) -> llvm::DominatorTree & {
      auto &FAM = GET_FUNCTION_ANALYSIS_MANAGER(MAM, M);
      return FAM.getResult<llvm::DominatorTreeAnalysis>(F);
    };
    ProxyInsertion::GetBoundsChecks get_bounds_checks =
        [&](llvm::Function &F) -> llvm::memoir::BoundsCheckResult & {
      auto &FAM = GET_FUNCTION_ANALYSIS_MANAGER(MAM, M);
      return FAM.getResult<llvm::memoir::BoundsCheckAnalysis>(F);
    };
    ProxyInsertion proxies(M, get_dominator_tree, get_bounds_checks);
  }

  MemOIRInst::invalidate();

  // Perform selection monomorphization.
  //  { SelectionMonomorphization monomorph(M); }

  return llvm::PreservedAnalyses::none();
}

} // namespace folio
