#include "memoir/analysis/BoundsCheckAnalysis.hpp"
#include "memoir/raising/ExtendedSSAConstruction.hpp"
#include "memoir/raising/RepairSSA.hpp"
#include "memoir/support/Casting.hpp"
#include "memoir/support/DataTypes.hpp"
#include "memoir/support/FetchAnalysis.hpp"
#include "memoir/support/PassUtils.hpp"
#include "memoir/support/Print.hpp"
#include "memoir/transforms/utilities/ReifyTempArgs.hpp"
#include "memoir/utility/Metadata.hpp"

#include "folio/Pass.hpp"
#include "folio/ProxyInsertion.hpp"
#include "folio/SelectionMonomorphization.hpp"

using namespace memoir;

namespace folio {

llvm::PreservedAnalyses FolioPass::run(llvm::Module &M,
                                       llvm::ModuleAnalysisManager &MAM) {

  // Construct analysis fetchers.
  auto &FAM = GET_FUNCTION_ANALYSIS_MANAGER(MAM, M);
  FetchAnalysis<llvm::DominatorTreeAnalysis, llvm::Function> get_dominator_tree{
    FAM
  };
  FetchAnalysis<memoir::BoundsCheckAnalysis, llvm::Function> get_bounds_checks{
    FAM
  };

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

  // Cleanup tempargs and stack variables.
  { memoir::reify_tempargs(M); }

  return llvm::PreservedAnalyses::none();
}

} // namespace folio
