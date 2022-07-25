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

std::string MustReadSummary::toString(std::string indent) const {
  std::string str, call_str;
  llvm::raw_string_ostream call_ss(call_str);
  call_ss << this->getCallInst();
  str = "(must read: \n" + indent + "  LLVM: " + call_ss.str() + "\n" + indent
        + "  field: " + this->getField().toString(indent + "         ") + "\n"
        + indent + ")";

  return str;
}

std::string MustWriteSummary::toString(std::string indent) const {
  std::string str, call_str;
  llvm::raw_string_ostream call_ss(call_str);
  call_ss << this->getCallInst();
  str = "(must write: \n" + indent + "  LLVM: " + call_ss.str() + "\n" + indent
        + "  field: " + this->getField().toString(indent + "         ") + "\n"
        + indent + ")";

  return str;
}

std::string MayReadSummary::toString(std::string indent) const {
  std::string str, call_str;
  llvm::raw_string_ostream call_ss(call_str);
  call_ss << this->getCallInst();
  str = "(may read: \n" + indent + "  LLVM: " + call_ss.str() + "\n";
  for (auto iter = this->cbegin(); iter != this->cend(); ++iter) {
    auto read_summary = *iter;
    str += indent + read_summary->toString(indent + "  ") + "\n";
  }
  str += indent + ")";

  return str;
}

std::string MayWriteSummary::toString(std::string indent) const {
  std::string str, call_str;
  llvm::raw_string_ostream call_ss(call_str);
  call_ss << this->getCallInst();
  str += "(may write: \n" + indent + "  LLVM: " + call_ss.str() + "\n";
  for (auto iter = this->cbegin(); iter != this->cend(); ++iter) {
    auto write_summary = *iter;
    str += indent + write_summary->toString(indent + "  ") + "\n";
  }
  str += indent + ")";

  return str;
}

} // namespace llvm::memoir
