#include "folio/opportunities/Analysis.hpp"

namespace folio {

Opportunities OpportunityAnalysis::run(llvm::Module &M,
                                       llvm::ModuleAnalysisManager &MAM) {
  Opportunities result;

#define OPPORTUNITY(NAME)                                                      \
  {                                                                            \
    auto _opportunities = MAM.getResult<NAME##Analysis>(M);                    \
    result.insert(result.end(), _opportunities.begin(), _opportunities.end()); \
  }
#include "folio/opportunities/Opportunities.def"

  return result;
}

} // namespace folio
