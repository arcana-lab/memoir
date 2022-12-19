#include "memoir/ir/Instructions.hpp"

namespace llvm::memoir {

llvm::Value &DeleteInst::getObjectOperand() const {
  return this->getObjectOperandAsUse().get();
}

llvm::Use &DeleteInst::getObjectOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

std::string DeleteInst::toString(std::string indent = "") const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "DeleteInst: " + llvm_str;

  return str;
}

} // namespace llvm::memoir
