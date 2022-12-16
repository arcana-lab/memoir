#include "common/ir/Instructions.hpp"

namespace llvm::memoir {

Type &AssertTypeInst::getType() const {
  // TODO
  return;
}

llvm::Value &AssertTypeInst::getTypeOperand() const {
  return this->getTypeOperandAsUse().get();
}

llvm::Use &AssertTypeInst::getTypeOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

llvm::Value &AssertTypeInst::getObjectOperand() const {
  return this->getObjectOperandAsUse().get();
}

llvm::Use &AssertTypeInst::getObjectOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(1);
}

std::string AssertTypeInst::toString(std::string indent = "") const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "AssertTypeInst: " + llvm_str;

  return str;
}

} // namespace llvm::memoir
