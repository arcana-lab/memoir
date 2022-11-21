#include "common/analysis/AccessAnalysis.hpp"

namespace llvm::memoir {

std::ostream &operator<<(std::ostream &os, const StructSummary &summary) {
  os << summary.toString();
  return os;
}

llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                              const StructSummary &summary) {
  os << summary.toString();
  return os;
}

std::string BaseStructSummary::toString(std::string indent) const {
  std::string str;

  str = "(base struct\n";
  str += indent + this->getAllocation().toString(indent) + "\n";
  str += indent + ")\n";

  return str;
}

std::string NestedStructSummary::toString(std::string indent) const {
  std::string str;

  str = "(nested struct\n";
  str += indent + "  container: \n";
  str +=
      indent + "    " + this->getContainer().toString(indent + "    ") + "\n";
  str +=
      indent + "  field index: " + std::to_string(this->getFieldIndex()) + "\n";
  str += indent + ")";

  return str;
}

std::string ContainedStructSummary::toString(std::string indent) const {
  std::string str;

  str = "(contained struct\n";
  str += indent + "  container: \n";
  str +=
      indent + "    " + this->getContainer().toString(indent + "    ") + "\n";
  str += indent + ")";

  return str;
}

std::string ReferencedStructSummary::toString(std::string indent) const {
  std::string str;

  str = "(referenced struct\n";
  str += indent + "  access: \n";
  str += indent + "    " + this->getAccess().toString(indent + "    ") + "\n";
  str += indent + ")";

  return str;
}

std::string ControlPHIStructSummary::toString(std::string indent) const {
  std::string str;

  str = "(control phi struct\n";
  for (auto idx = 0; idx < this->getNumIncoming(); idx++) {
    auto &incoming_struct = this->getIncomingStruct(idx);
    auto &incoming_bb = this->getIncomingBlock(idx);
    str += indent + "  [ " + incoming_struct.toString(indent + "    ");
    str += "\n";
    str += indent + "   ," + incoming_bb.getName().str();
    str += "\n";
    str += indent + "  ],\n";
  }
  str += indent + ")";

  return str;
}

std::string CallPHIStructSummary::toString(std::string indent) const {
  std::string str;

  str = "(call phi struct\n";
  for (auto idx = 0; idx < this->getNumIncoming(); idx++) {
    auto &incoming_struct = this->getIncomingStruct(idx);
    auto &incoming_call = this->getIncomingCall(idx);
    s

        std::string call_str;
    lvm::raw_string_ostream call_ss(call_str);
    call_ss << incoming_call;

    str += indent + "  [ " + incoming_struct.toString(indent + "    ");
    str += "\n";
    str += indent + "   ," + call_str;
    str += "\n";
    str += indent + "  ],\n";
  }
  str += indent + ")";

  return str;
}

} // namespace llvm::memoir
