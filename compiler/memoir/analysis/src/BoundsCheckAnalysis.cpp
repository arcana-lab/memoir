#include "memoir/analysis/BoundsCheckAnalysis.hpp"

namespace llvm::memoir {

BoundsCheckResult BoundsCheckAnalysis::run(llvm::Function &F,
                                           llvm::FunctionAnalysisManager &FAM) {
  BoundsCheckResult result;

  // Perform a data flow analysis.

  return result;
}

llvm::AnalysisKey BoundsCheckAnalysis::Key;

} // namespace llvm::memoir
