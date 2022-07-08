#include "TypeAnalysis.hpp"

/*
 * TODO: add an actual printer implementation
 */

namespace llvm::memoir {

std::string StructTypeSummary::toString() {
  return "(struct)";
}

std::string TensorTypeSummary::toString() {
  return "(tensor)";
}

std::string IntegerTypeSummary::toString() {
  return "(integer)";
}

std::string FloatTypeSummary::toString() {
  return "(float)";
}

std::string DoubleTypeSummary::toString() {
  return "(double)";
}

std::string ReferenceTypeSummary::toString() {
  return "(reference)";
}

} // namespace llvm::memoir
