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

std::string ReadSummary::toString(std::string indent) const {
  std::string str, call_str;

  llvm::raw_string_ostream call_ss(call_str);
  call_ss << this->getCallInst();

  if (this->isMust()) {
    str = "(must read: \n";
    str += indent + "  LLVM: " + call_ss.str() + "\n";
    str += indent + "  field: \n";
    str += indent + "    " + this->getField().toString(indent + "    ") + "\n";
    str += indent + ")";
  } else {
    str += "(may write: \n";
    str += indent + "  LLVM: " + call_ss.str() + "\n";
    for (auto iter = this->cbegin(); iter != this->cend(); ++iter) {
      auto write_summary = *iter;
      str += indent + "  " + write_summary->toString(indent + "    ") + "\n";
    }
    str += indent + ")";
  }

  return str;
}

std::string WriteSummary::toString(std::string indent) const {
  std::string str, call_str;

  llvm::raw_string_ostream call_ss(call_str);
  call_ss << this->getCallInst();

  if (this->isMust()) {
    str = "(must write: \n";
    str += indent + "  LLVM: " + call_ss.str() + "\n";
    str += indent + "  field: \n";
    str += indent + "    " + this->getField().toString(indent + "    ") + "\n";
    str += indent + ")";
  } else {
    str = "(may write: \n";
    str += indent + "  LLVM: " + call_str + "\n";
    str += indent + "  fields: \n";
    for (auto iter = this->cbegin(); iter != this->cend(); ++iter) {
      auto read_summary = *iter;
      str += indent + "    " + read_summary->toString(indent + "    ") + "\n";
    }
    str += indent + ")";
  }

  return str;
}

} // namespace llvm::memoir
