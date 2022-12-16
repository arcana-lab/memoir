#include "common/ir/Instructions.hpp"

#include "common/support/Assert.hpp"

namespace llvm::memoir {

llvm::Value &SliceInst::getSlicedObject() const {
  return this->getCallInst();
}

llvm::Value &DeleteInst::getObjectOperand() const {
  return this->getObjectOperandAsUse().get();
}

llvm::Use &DeleteInst::getObjectOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

llvm::Value &DeleteInst::getBeginIndex() const {
  return this->getBeginIndexAsUse().get();
}

llvm::Use &DeleteInst::getBeginIndexAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

llvm::Value &DeleteInst::getEndIndex() const {
  return this->getEndIndexAsUse().get();
}

llvm::Use &DeleteInst::getEndIndexAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

std::string SliceInst::toString(std::string indent = "") const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "SliceInst: " + llvm_str;

  return str;
}

} // namespace llvm::memoir
