#include "llvm/Pass.h"

#include "memoir/support/InternalDatatypes.hpp"

#include "folio/analysis/ContentSummary.hpp"

namespace folio {

using Contents = typename llvm::memoir::map<llvm::Value *, Content *>;

class ContentAnalysis : public llvm::AnalysisInfoMixin<ContentAnalysis> {
  friend struct llvm::AnalysisInfoMixin<ContentAnalysis>;
  llvm::AnalysisKey Key;

public:
  using Result = typename folio::Contents;
  Result run(llvm::Module &M, llvm::ModuleAnalysisManager &MAM);
};

} // namespace folio
