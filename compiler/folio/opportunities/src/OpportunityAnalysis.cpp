#include "folio/analysis/ContentAnalysis.hpp"

#include "folio/opportunities/Analysis.hpp"

#include "memoir/support/InternalDatatypes.hpp"

using namespace llvm::memoir;

namespace folio {

static llvm::cl::list<std::string> disabled_opportunities(
    "disable-opportunities",
    llvm::cl::desc("List of opportunities to disable."),
    llvm::cl::ZeroOrMore,
    llvm::cl::CommaSeparated);

Opportunities OpportunityAnalysis::run(llvm::Module &M,
                                       llvm::ModuleAnalysisManager &MAM) {

  // Initialize the empty result.
  Opportunities result;

  // Run ContentsAnalysis.
  auto &contents = MAM.getResult<ContentAnalysis>(M);

  // Initialize the ProxyManager.
  ProxyManager::initialize(contents);

  // Collect the opportunity blacklist.
  set<std::string> blacklist = {};
  for (auto &opportunity : disabled_opportunities) {
    blacklist.insert(opportunity);
  }

  // Discover all opportunities.
#define OPPORTUNITY(CLASS)                                                     \
  if (blacklist.count(#CLASS) == 0) {                                          \
    auto _opportunities = MAM.getResult<CLASS##Analysis>(M);                   \
    result.insert(result.end(), _opportunities.begin(), _opportunities.end()); \
  }
#include "folio/opportunities/Opportunities.def"

  // Add all proxies discovered/required as opportunities.
  for (auto *proxy : ProxyManager::proxies()) {
    // Wrap the Proxy in a ProxyOpportunity.
    result.push_back(new ProxyOpportunity(*proxy));
  }

  return result;
}

llvm::AnalysisKey OpportunityAnalysis::Key;

} // namespace folio
