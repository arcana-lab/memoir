#include "memoir/utility/Metadata.hpp"

#include "memoir/analysis/BoundsCheckAnalysis.hpp"
#include "memoir/raising/ExtendedSSAConstruction.hpp"
#include "memoir/support/Casting.hpp"
#include "memoir/support/DataTypes.hpp"
#include "memoir/support/PassUtils.hpp"
#include "memoir/support/Print.hpp"

#include "memoir/support/FetchAnalysis.hpp"

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

  // Construct analysis fetchers.
  auto &FAM = GET_FUNCTION_ANALYSIS_MANAGER(MAM, M);
  FetchAnalysis<llvm::DominatorTreeAnalysis, llvm::Function> get_dominator_tree{
    FAM
  };
  FetchAnalysis<llvm::memoir::BoundsCheckAnalysis, llvm::Function>
      get_bounds_checks{ FAM };

  // Transform the program to Extended SSA form.
  {
    for (auto &F : M) {
      if (not F.empty()) {
        construct_extended_ssa(F, get_dominator_tree(F));
      }
    }
  }

  // Insert proxies and encode uses.
  {
    ProxyInsertion proxies(M, get_dominator_tree, get_bounds_checks);
    MemOIRInst::invalidate();
  }

  // Perform selection monomorphization.
  //  { SelectionMonomorphization monomorph(M); }

  return llvm::PreservedAnalyses::none();
}

} // namespace folio
