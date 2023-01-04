#include "memoir/analysis/CollectionAnalysis.hpp"

namespace llvm::memoir {

/*
 * Initialization.
 */
CollectionAnalysis::CollectionAnalysis(llvm::Module &M) : M(M) {
  // Do nothing.
}

/*
 * Queries
 */
Collection *CollectionAnalysis::analyze(llvm::Use &U) {
  return CollectionAnalysis::get().getCollection(U);
}

Collection *CollectionAnalysis::analyze(llvm::Argument &A) {
  return CollectionAnalysis::get().getCollection(A);
}

/*
 * Analysis
 */
Collection *CollectionAnalysis::getCollection(llvm::Use &U) {
  return nullptr;
}

Collection *CollectionAnalysis::getCollection(llvm::Argument &A) {
  return nullptr;
}

/*
 * Utility
 */

/*
 * Management
 */
CollectionAnalysis &CollectionAnalysis::get() {
  CollectionAnalysis CA;
  return CA;
}

void CollectionAnalysis::invalidate() {
  CollectionAnalysis::get()._invalidate();
}

void CollectionAnalysis::_invalidate() {
  return;
}

} // namespace llvm::memoir
