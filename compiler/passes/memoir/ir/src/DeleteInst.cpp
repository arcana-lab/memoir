#include "memoir/ir/Instructions.hpp"

namespace llvm::memoir {

Struct &DeleteStructInst::getStructDeleted() const {
  // TODO: run StructAnalysis.
  return;
}

llvm::Value &DeleteStructInst::getStructOperand() const {
  return *(this->getStructOperandAsUse().get());
}

llvm::Use &DeleteStructInst::getStructOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

std::string DeleteStructInst::toString(std::string indent) const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "DeleteStructInst: " + llvm_str;

  return str;
}

llvm::Value &DeleteCollectionInst::getCollectionOperand() const {
  return *(this->getCollectionOperandAsUse().get());
}

llvm::Use &DeleteCollectionInst::getCollectionOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

std::string DeleteCollectionInst::toString(std::string indent) const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "DeleteCollectionInst: " + llvm_str;

  return str;
}

} // namespace llvm::memoir
