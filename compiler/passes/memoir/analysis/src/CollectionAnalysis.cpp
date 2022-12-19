#include "memoir/analysis/CollectionAnalysis.hpp"

namespace llvm::memoir {

/*
 * Top-level initialization.
 */
CollectionAnalysis::CollectionAnalysis(llvm::Module &M) : M(M) {
  /*
   * Initialize the analysis.
   */
  this->initialize();

  /*
   * Perform the analysis.
   */
  this->analyze();
}

void CollectionAnalysis::initialize() {
  errs() << "ColletionAnalysis: initialize.\n";

  /*
   * Perform all needed initialization for the CollectionAnalysis pass.
   */
  return;
}

/*
 * Top-level analysis.
 */
void CollectionAnalysis::analyze() {
  errs() << "CollectionAnalysis: analyzing.\n";

  /*
   * Analyze all of the fields in the program.
   */
  analyzeFields();

  return;
}

/*
 * Analysis drivers
 */
void CollectionAnalysis::analyzeFields() {
  errs() << "CollectionAnalysis: analyzing fields\n";

  return;
}

/*
 * Qeueries
 */
CollectionSummary *CollectionAnalysis::getCollectionSummary(
    llvm::Value &value) {

  return nullptr;
}

/*
 * Utility
 */

/*
 * Management
 */
CollectionAnalysis &CollectionAnalysis::get(Module &M) {
  auto found_analysis = CollectionAnalysis::analyses.find(&M);
  if (found_analysis != CollectionAnalysis::analyses.end()) {
    return *(found_analysis->second);
  }

  auto new_analysis = new CollectionAnalysis(M);
  CollectionAnalysis::analyses[&M] = new_analysis;
  return *new_analysis;
}

void CollectionAnalysis::invalidate(Module &M) {
  auto found_analysis = CollectionAnalysis::analyses.find(&M);
  if (found_analysis != CollectionAnalysis::analyses.end()) {
    auto &analysis = *(found_analysis->second);
    analysis.invalidate();
  }

  return;
}

void CollectionAnalysis::invalidate() {
  return;
}

} // namespace llvm::memoir
