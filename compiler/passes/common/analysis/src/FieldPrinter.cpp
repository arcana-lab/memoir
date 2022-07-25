#include "common/analysis/AccessAnalysis.hpp"

#include <sstream>

namespace llvm::memoir {

std::ostream &operator<<(std::ostream &os, const FieldSummary &summary) {
  os << summary.toString();
  return os;
}

llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                              const FieldSummary &summary) {
  os << summary.toString();
  return os;
}

std::string StructFieldSummary::toString(std::string indent) const {
  std::string str;
  str = "(struct field: \n" + indent
        + "  type: " + this->getType().toString(indent + "          ") + "\n"
        + indent + "  allocation: "
        + this->pointsTo().toString(indent + "                ") + "\n" + indent
        + "  index: " + std::to_string(this->getIndex()) + "\n" + indent + ")";

  return str;
}

std::string TensorElementSummary::toString(std::string indent) const {
  std::string str;
  str = "(struct field: \n" + indent
        + "  type: " + this->getType().toString(indent + "        ") + "\n"
        + indent + "  allocation: "
        + this->pointsTo().toString(indent + "              ") + "\n" + indent
        + "  indices: " + "\n";
  for (auto i = 0; i < this->getNumberOfDimensions(); i++) {
    std::string value_str;
    llvm::raw_string_ostream value_ss(value_str);
    value_ss << this->getIndex(i);

    str +=
        indent + "    dimension " + std::to_string(i) + ": " + value_str + "\n";
  }
  str += indent + ")";

  return str;
}

} // namespace llvm::memoir
