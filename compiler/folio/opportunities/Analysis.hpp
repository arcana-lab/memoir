#ifndef FOLIO_ANALYSIS_H
#define FOLIO_ANALYSIS_H

#include "folio/opportunities/Opportunities.hpp"

#include "llvm/Pass.h"

#include "llvm/IR/PassManager.h"

#include "llvm/Support/CommandLine.h"

namespace folio {

// Declare the Analysis for each opportunity
#define OPPORTUNITY(CLASS)                                                     \
  class CLASS##Analysis : public llvm::AnalysisInfoMixin<CLASS##Analysis> {    \
    friend struct llvm::AnalysisInfoMixin<CLASS##Analysis>;                    \
                                                                               \
    static llvm::AnalysisKey Key;                                              \
                                                                               \
  public:                                                                      \
    using Result = typename folio::Opportunities;                              \
                                                                               \
    Result run(llvm::Module &M, llvm::ModuleAnalysisManager &MAM);             \
  };
#include "folio/opportunities/Opportunities.def"

// Declare the top-level OpportunityAnalysis.
class OpportunityAnalysis
  : public llvm::AnalysisInfoMixin<OpportunityAnalysis> {
  friend struct llvm::AnalysisInfoMixin<OpportunityAnalysis>;

  static llvm::AnalysisKey Key;

public:
  using Result = typename folio::Opportunities;

  Result run(llvm::Module &M, llvm::ModuleAnalysisManager &MAM);
};

} // namespace folio

#endif // FOLIO_ANALYSIS_H
