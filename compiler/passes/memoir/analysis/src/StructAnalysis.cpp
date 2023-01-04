#include "memoir/analysis/StructAnalysis.hpp"

namespace llvm::memoir {

/*
 * Initialization
 */
StructAnalysis::StructAnalysis() {
  // Do nothing.
}

/*
 * Queries
 */
Struct *StructAnalysis::analyze(llvm::Value &V) {
  return StructAnalysis::get().getStruct(V);
}

/*
 * Analysis
 */
Struct StructAnalysis::getStruct(llvm::Value &V) {
  return nullptr;
}

/*
 * Helper functions
 */

/*
 * Management
 */
StructAnalysis &StructAnalysis::get() {
  StructAnalysis SA;
  return SA;
}

void StructAnalysis::invalidate() {
  StructAnalysis::get()._invalidate();
}

} // namespace llvm::memoir
