#include "common/analysis/AccessAnalysis.hpp"
#include "common/utility/Metadata.hpp"

namespace llvm::memoir {

/*
 * Top-level initialization.
 */
AccessAnalysis::AccessAnalysis(Module &M)
  : M(M),
    type_analysis(TypeAnalysis::get(M)),
    allocation_analysis(AllocationAnalysis::get(M)) {
  /*
   * Initialize the analysis.
   */
  initialize();

  /*
   * Perform the analysis.
   */
  analyze();
}

void AccessAnalysis::initialize() {
  errs() << "AccessAnalysis: initialize.\n";

  /*
   * Perform all needed initialization for the AccessAnalysis pass.
   */
  return;
}

/*
 * Top-level analysis.
 */
void AccessAnalysis::analyze() {
  errs() << "AccessAnalysis: analyzing.\n";

  /*
   * Analyze all of the objects present in the program, both allocations and
   * nested objects.
   */
  analyzeStructs();

  /*
   * Reconcile the objects that are accessed through references.
   */
  reconcileReferences();

  /*
   * Analyze the accesses within the program.
   */
  analyzeAccesses();

  errs() << "AccessAnalysis: done.\n";

  return;
}

/*
 * Management
 */
AccessAnalysis &AccessAnalysis::get(Module &M) {
  auto found_analysis = AccessAnalysis::analyses.find(&M);
  if (found_analysis != AccessAnalysis::analyses.end()) {
    return *(found_analysis->second);
  }

  auto new_analysis = new AccessAnalysis(M);
  AccessAnalysis::analyses[&M] = new_analysis;
  return *new_analysis;
}

void AccessAnalysis::invalidate(Module &M) {
  auto found_analysis = AccessAnalysis::analyses.find(&M);
  if (found_analysis != AccessAnalysis::analyses.end()) {
    auto &analysis = *(found_analysis->second);
    analysis.invalidate();
  }

  return;
}

void AccessAnalysis::invalidate() {
  this->access_summaries.clear();

  return;
}

map<llvm::Module *, AccessAnalysis *> AccessAnalysis::analyses = {};

} // namespace llvm::memoir
