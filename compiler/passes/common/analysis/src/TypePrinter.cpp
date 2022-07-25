#include "common/analysis/TypeAnalysis.hpp"

#include <sstream>

/*
 * TODO: add an actual printer implementation
 */

namespace llvm::memoir {

std::ostream &operator<<(std::ostream &os, const TypeSummary &summary) {
  os << summary.toString();
  return os;
}

llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                              const TypeSummary &summary) {
  os << summary.toString();
  return os;
}

std::string StructTypeSummary::toString(std::string indent) const {
  return "(struct)";
}

std::string TensorTypeSummary::toString(std::string indent) const {
  return "(tensor)";
}

std::string IntegerTypeSummary::toString(std::string indent) const {
  std::string str = "";
  if (this->is_signed) {
    str += "u";
  }
  str += "int";
  str += std::to_string(this->bitwidth);
  return str;
}

std::string FloatTypeSummary::toString(std::string indent) const {
  return "float";
}

std::string DoubleTypeSummary::toString(std::string indent) const {
  return "double";
}

std::string ReferenceTypeSummary::toString(std::string indent) const {
  std::string str = "";
  str += "(reference: ";
  str += indent + "  " + this->referenced_type.toString(indent + "  ") + "\n";
  str += indent + ")";
}

} // namespace llvm::memoir
