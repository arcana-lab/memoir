#include "common/analysis/AllocationAnalysis.hpp"

namespace llvm::memoir {

std::string StructAllocationSummary::toString() {
  return "(struct\n" + "  type: " + this->type_summary.toString() + ")";
}

std::string TensorAllocationSummary::toString() {
  auto &element_type_str = this->element_type_summary.toString();
  return "(tensor\n" + "  type: " + element_type_str + "\n"
         + "  # dimensions: " + this->length_of_dimensions.size() + ")";
}

} // namespace llvm::memoir
