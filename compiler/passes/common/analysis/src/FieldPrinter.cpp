#include "common/analysis/AccessAnalysis.hpp"

namespace llvm::memoir {

std::ostream &FieldSummary::operator<<(std::ostream &os,
                                       const FieldSummary &summary) {
  os << summary.toString();
  return os;
}

std::string StructFieldSummary::toString(std::string indent) {
  std::stringstream sstream;
  sstream << "(struct field: " << std::endl
          << indent
          << "  type: " << this->getType().toString(indent + "          ")
          << std::endl
          << indent << "  allocation: "
          << this->pointsTo().toString(indent + "                ") << std::endl
          << indent << "  index: " << this->getIndex() << std::endl
          << indent << ")";

  return sstream.str();
}

std::string TensorElementSummary::toString(std::string indent) {
  std::stringstream sstream;
  sstream << "(struct field: " << std::endl
          << indent
          << "  type: " << summary.getType().toString(indent + "        ")
          << std::endl
          << indent << "  allocation: "
          << summary.pointsTo().toString(indent + "              ") << std::endl
          << indent << "  indices: " << std::endl;
  for (auto i = 0; i < summary.getNumberOfDimensions(); i++) {
    sstream << indent << "    dimension " << i << ": " << summary.getIndex(i)
            << std::endl;
  }
  sstream << indent << ")";

  return sstream.str();
}

} // namespace llvm::memoir
