#ifndef MEMOIR_SUPPORT_FETCHANALYSIS_H
#define MEMOIR_SUPPORT_FETCHANALYSIS_H

#include "llvm/IR/PassManager.h"

namespace memoir {

template <typename Analysis, typename IRUnit>
concept Analyzable =
    requires(Analysis &a, IRUnit &ir, llvm::AnalysisManager<IRUnit> &am) {
      { a.run(ir, am) } -> std::same_as<typename Analysis::Result>;
    };

template <typename Analysis, typename IRUnit, bool Cached = false>
  requires Analyzable<Analysis, IRUnit>
struct FetchAnalysis {
  using Manager = llvm::AnalysisManager<IRUnit>;

  FetchAnalysis(Manager &am) : am(am) {}

  Analysis::Result &operator()(IRUnit &ir) {
    if constexpr (Cached) {
      return *this->am.template getCachedResult<Analysis>(ir);
    } else {
      return this->am.template getResult<Analysis>(ir);
    }
  };

protected:
  Manager &am;
};

} // namespace memoir

#endif // MEMOIR_SUPPORT_FETCHANALYSIS_H
