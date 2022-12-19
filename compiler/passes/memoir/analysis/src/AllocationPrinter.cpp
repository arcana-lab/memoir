#include "memoir/analysis/AllocationAnalysis.hpp"

#include <sstream>

namespace llvm::memoir {

std::ostream &operator<<(std::ostream &os, const AllocationSummary &summary) {
  os << summary.toString();
  return os;
}

llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                              const AllocationSummary &summary) {
  os << summary.toString();
  return os;
}

std::string StructAllocationSummary::toString(std::string indent) const {
  std::string str, call_str;
  llvm::raw_string_ostream call_ss(call_str);
  call_ss << this->getCallInst();

  str = "(struct \n" + indent + "  LLVM: " + call_ss.str() + "\n";
  str += indent + "  type: \n";
  str += indent + "  " + this->getType().toString(indent + "  ") + "\n";
  str += indent + ")";

  return str;
}

std::string TensorAllocationSummary::toString(std::string indent) const {
  std::string str, call_str;
  llvm::raw_string_ostream call_ss(call_str);
  call_ss << this->getCallInst();

  str = "(tensor \n";
  str += indent + "  LLVM: " + call_ss.str() + "\n";
  str += indent + "  dimensions: \n";
  int i = 0;
  for (auto length : this->length_of_dimensions) {
    std::string length_str;
    llvm::raw_string_ostream length_ss(length_str);
    length_ss << *length;

    str +=
        indent + "    dimension " + std::to_string(i) + ": " + call_str + "\n";
    i++;
  }
  str += indent + ")";

  return str;
}

std::string AssocArrayAllocationSummary::toString(std::string indent) const {
  std::string str, call_str;
  llvm::raw_string_ostream call_ss(call_str);
  call_ss << this->getCallInst();

  str = "(associative array\n";
  str += indent + "LLVM: " + call_ss.str() + "\n";
  str += indent + "Key Type: \n";
  str += indent + "  " + this->getKeyType().toString(indent + "  ") + "\n";
  str += indent + "Value Type: \n";
  str += indent + "  " + this->getValueType().toString(indent + "  ") + "\n";
  str += indent + ")\n";

  return str;
}

std::string SequenceAllocationSummary::toString(std::string indent) const {
  std::string str, call_str;
  llvm::raw_string_ostream call_ss(call_str);
  call_ss << this->getCallInst();

  str = "(sequence\n";
  str += indent + "LLVM: " + call_ss.str() + "\n";
  str += indent + "Element Type: \n";
  str += indent + "  " + this->getElementType().toString(indent + "  ") + "\n";
  str += indent + ")\n";

  return str;
}

} // namespace llvm::memoir
