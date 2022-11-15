#include "common/analysis/AccessAnalysis.hpp"

namespace llvm::memoir {

std::ostream &operator<<(std::ostream &os, const ObjectSummary &summary) {
  os << summary.toString();
  return os;
}

llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                              const ObjectSummary &summary) {
  os << summary.toString();
  return os;
}

std::string BaseObjectSummary::toString(std::string indent) const {
  std::string str;

  str = this->allocation.toString(indent);

  return str;
}

std::string NestedObjectSummary::toString(std::string indent) const {
  std::string str;

  str = this->field.toString(indent);

  return str;
}

} // namespace llvm::memoir
