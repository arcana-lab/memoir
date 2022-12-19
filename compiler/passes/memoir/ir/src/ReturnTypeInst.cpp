#include "memoir/ir/Instructions.hpp"

namespace llvm::memoir {

Type &ReturnTypeInst::getType() const {
  // TODO
  return;
}

llvm::Value &ReturnTypeInst::getTypeOperand() const {
  return this->getTypeOperandAsUse().get();
}

llvm::Use &ReturnTypeInst::getTypeOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

std::string ReturnTypeInst::toString(std::string indent = "") const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "ReturnTypeInst: " + llvm_str;

  return str;
}

} // namespace llvm::memoir
