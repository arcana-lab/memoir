#include "folio/opportunities/Analysis.hpp"

namespace folio {

Opportunities run(llvm::Module &M, llvm::ModuleAnalysisManager &MAM) {
  Opportunities result;

  // Discover all proxy opportunities in the program.
  // TODO

  // Fetch the Content Analysis results.

  return result;
}

llvm::AnalysisKey ProxyOpportunityAnalysis::Key;

} // namespace folio
