#include "common/analysis/AccessAnalysis.hpp"

namespace llvm::memoir {

std::ostream &operator<<(std::ostream &os, const AccessSummary &summary) {
  os << summary.toString();
  return os;
}

llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                              const AccessSummary &summary) {
  os << summary.toString();
  return os;
}

std::string IndexedReadSummary::toString(std::string indent) const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "(read: \n";
  str += indent + "  LLVM: " + llvm_str + "\n";
  str += indent + "  index: (\n";
  for (auto dim = 0; dim < this->getNumDimensions(); dim++) {
    std::string llvm_str;
    llvm::raw_string_ostream llvm_ss(llvm_str);
    llvm_ss << this->getIndexValue(dim);
    str += indent + "    dimension " + std::to_string(dim) + ": " + llvm_str
           + "\n";
  }
  str += indent + "  )\n";
  str += indent + "  collection: \n"
         + this->getCollection().toString(indent + "    ");
  str += indent + "\n)";
  return str;
}

std::string IndexedWriteSummary::toString(std::string indent) const {
  std::string str, call_str, write_str;
  llvm::raw_string_ostream call_ss(call_str);
  call_ss << this->getCallInst();
  llvm::raw_string_ostream write_ss(write_str);
  write_ss << this->getValueWritten();

  str = "(write: \n";
  str += indent + "  LLVM: " + llvm_str + "\n";
  str += indent + "  written: " + write_str + "\n";
  str += indent + "  index: (\n";
  for (auto dim = 0; dim < this->getNumDimensions(); dim++) {
    std::string llvm_str;
    llvm::raw_string_ostream llvm_ss(llvm_str);
    llvm_ss << this->getIndexValue(dim);
    str += indent + "    dimension " + std::to_string(dim) + ": " + llvm_str
           + "\n";
  }
  str += indent + "  )\n";
  str += indent + "  collection: \n"
         + this->getCollection().toString(indent + "    ");
  str += indent + "\n)";
  return str;
}

std::string AssocReadSummary::toString(std::string indent) const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "(read: \n";
  str += indent + "  LLVM: " + llvm_str + "\n";
  str += indent + "  key: \n";
  str += indent + "    " + this->getKey().toString(indent + "    ");
  str += indent + "  collection: \n";
  str += indent + "    " + this->getCollection().toString(indent + "    ");
  str += indent + "\n)";
  return str;
}

std::string AssocWriteSummary::toString(std::string indent) const {
  std::string str, call_str, write_str;
  llvm::raw_string_ostream call_ss(call_str);
  call_ss << this->getCallInst();
  llvm::raw_string_ostream write_ss(write_str);
  write_ss << this->getValueWritten();

  str = "(write: \n";
  str += indent + "  LLVM: " + llvm_str + "\n";
  str += indent + "  written: " + write_str + "\n";
  str += indent + "  key: \n";
  str += indent + "    " + this->getKey().toString(indent + "    ");
  str += indent + "  collection: \n";
  str += indent + "    " + this->getCollection().toString(indent + "    ");
  str += indent + "\n)";
  return str;
}

std::string SliceReadSummary::toString(std::string indent) const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();
  llvm::raw_string_ostream left_ss(llvm_str);
  left_ss << this->getLeft();
  llvm::raw_string_ostream right_ss(llvm_str);
  right_ss << this->getRight();

  str = "(read: \n";
  str += indent + "  LLVM: " + llvm_str + "\n";
  str += indent + "  slice: (\n";
  str += indent + "    " + left_str + "\n";
  str += indent + "    :\n";
  str += indent + "    " + right_str + "\n";
  str += indent + "  )\n";
  str += indent + "  collection: \n"
         + this->getCollection().toString(indent + "    ");
  str += indent + "\n)";
  return str;
}

std::string SliceWriteSummary::toString(std::string indent) const {
  std::string str, call_str, write_str;
  llvm::raw_string_ostream call_ss(call_str);
  call_ss << this->getCallInst();
  llvm::raw_string_ostream write_ss(write_str);
  write_ss << this->getValueWritten();

  str = "(write: \n";
  str += indent + "  LLVM: " + llvm_str + "\n";
  str += indent + "  written: " + write_str + "\n";
  str += indent + "  slice: (\n";
  str += indent + "    " + left_str + "\n";
  str += indent + "    :\n";
  str += indent + "    " + right_str + "\n";
  str += indent + "  )\n";
  str += indent + "  collection: \n"
         + this->getCollection().toString(indent + "    ");
  str += indent + "\n)";
  return str;
}

} // namespace llvm::memoir
