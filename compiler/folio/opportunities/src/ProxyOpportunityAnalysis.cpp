#include "folio/analysis/ContentAnalysis.hpp"

#include "folio/opportunities/Analysis.hpp"

namespace folio {

Opportunities run(llvm::Module &M, llvm::ModuleAnalysisManager &MAM) {

  // The set of opportunities discovered by this analysis.
  // NOTE: opportunities must be allocated
  Opportunities result;

  // Fetch the Content Analysis results.
  auto &contents = MAM.getResult<ContentAnalysis>(M);

  // Discover all proxy opportunities in the program.

  // For collection P to be a proxy of collection C:
  //  - \exists collection A, where domain(A) \subseteq domain(C)
  //  - P must be available at all uses of A
  //  - range(P) \contains domain(C)

  // TODO

  return result;
}

llvm::AnalysisKey ProxyOpportunityAnalysis::Key;

} // namespace folio
