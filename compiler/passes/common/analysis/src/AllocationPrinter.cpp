#include "common/analysis/AllocationAnalysis.hpp"

namespace llvm::memoir {

std::ostream AllocationSummary::operator<<(ostream &os,
                                           const AllocationSummary &summary) {
  os << summary.toString();
  return os;
}

std::string StructAllocationSummary::toString(std::string indent) {
  std::stringstream sstream;
  sstream << "(struct" << std::endl
          << indent << "  LLVM: " << this->getCallInst() << std::endl
          << indent
          << "  type: " << this->getTypeSummary().toString(indent + "        ")
          << std::endl
          << ")";

  return sstream.str();
}

std::string TensorAllocationSummary::toString(std::string indent) {
  std::stringstream sstream;
  sstream << "(tensor" << std::endl
          << indent << "  LLVM: " << this->getCallInst() << std::endl
          << indent << "  dimensions: " << std::endl;
  auto i = 0;
  for (auto length : this->length_of_dimensions) {
    sstream << indent << "    dimension " << i << ": " << *length << std::endl;
    i++;
  }
  sstream << ")";

  return sstream.str();
}

} // namespace llvm::memoir
