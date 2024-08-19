#include "folio/opportunities/Analysis.hpp"

namespace folio {

Opportunities run(llvm::Module &M, llvm::ModuleAnalysisManager &MAM) {

  // The set of opportunities discovered by this analysis.
  // NOTE: opportunities must be allocated
  Opportunities result;

  // Discover all proxy opportunities in the program.
  // TODO

  // Fetch the Content Analysis results.

  return result;
}

llvm::AnalysisKey ProxyOpportunityAnalysis::Key;

} // namespace folio
