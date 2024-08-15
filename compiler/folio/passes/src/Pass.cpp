#include "memoir/support/Casting.hpp"
#include "memoir/support/InternalDatatypes.hpp"
#include "memoir/support/Print.hpp"

#include "folio/analysis/ConstraintInference.hpp"
#include "folio/analysis/OpportunityDiscovery.hpp"

#include "folio/solver/Implementation.hpp"
#include "folio/solver/Solver.hpp"

#include "folio/passes/Pass.hpp"

using namespace llvm::memoir;

namespace folio {

llvm::PreservedAnalyses FolioPass::run(llvm::Module &M,
                                       llvm::ModuleAnalysisManager &MAM) {

  // Fetch the ConstraintInference results.
  auto &constraints = MAM.getResult<ConstraintInference>(M);

  // Collect all of the selectable variables.
  set<llvm::Value *> selectable = {};
  for (auto &F : M) {
    if (F.empty()) {
      continue;
    }

    for (auto &BB : F) {
      for (auto &I : BB) {
        auto *memoir_inst = into<MemOIRInst>(&I);
        if (not memoir_inst) {
          continue;
        }

        // For the time being, we will only consider explicit allocations as
        // selectable.
        if (auto *seq = dyn_cast<SequenceAllocInst>(memoir_inst)) {
          selectable.insert(&I);
        } else if (auto *assoc = dyn_cast<AssocAllocInst>(memoir_inst)) {
          selectable.insert(&I);
        }
      }
    }
  }

  // Fetch the OpportunityDiscovery results.
  // auto &opportunities = MAM.getResult<OpportunityDiscovery>(M);
  Opportunities opportunities;

  // Instantiate all available implementations.
  Implementations implementations;

  // Pass the analysis results to the solver.
  Solver(selectable, constraints, opportunities, implementations);

  // All done.
  return llvm::PreservedAnalyses::none();
}

} // namespace folio
