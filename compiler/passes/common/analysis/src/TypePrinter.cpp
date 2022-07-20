#include "TypeAnalysis.hpp"

/*
 * TODO: add an actual printer implementation
 */

namespace llvm::memoir {

std::ostream &TypeSummary::operator<<(std::ostream &os,
                                      const TypeSummary &summary) {
  os << summary.toString();
  return os;
}

std::string StructTypeSummary::toString(std::string indent) {
  return "(struct)";
}

std::string TensorTypeSummary::toString(std::string indent) {
  return "(tensor)";
}

std::string IntegerTypeSummary::toString(std::string indent) {
  return "(integer)";
}

std::string FloatTypeSummary::toString(std::string indent) {
  return "(float)";
}

std::string DoubleTypeSummary::toString(std::string indent) {
  return "(double)";
}

std::string ReferenceTypeSummary::toString(std::string indent) {
  return "(reference)";
}

} // namespace llvm::memoir
