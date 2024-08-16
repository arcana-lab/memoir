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
  Opportunities result;

  // Collect the opportunity blacklist.
  set<std::string> blacklist = {};
  for (auto &opportunity : disabled_opportunities) {
    blacklist.insert(opportunity);
  }

#define OPPORTUNITY(NAME)                                                      \
  if (blacklist.count(#NAME) == 0) {                                           \
    auto _opportunities = MAM.getResult<NAME##Analysis>(M);                    \
    result.insert(result.end(), _opportunities.begin(), _opportunities.end()); \
  }
#include "folio/opportunities/Opportunities.def"

  return result;
}

} // namespace folio
