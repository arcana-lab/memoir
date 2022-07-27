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

  str = "(base object\n";
  str += indent + "  allocation: " + this->allocation.toString(indent + "    ")
         + "\n";
  str += indent + ")";

  return str;
}

std::string NestedStructSummary::toString(std::string indent) const {
  std::string str;

  str = "(nested struct\n";
  str += indent + "  field: " + this->field.toString(indent + "    ") + "\n";
  str += indent + ")";

  return str;
}

} // namespace llvm::memoir
