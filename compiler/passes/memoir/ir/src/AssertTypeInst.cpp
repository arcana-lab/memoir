#include "memoir/ir/Instructions.hpp"

#include "memoir/analysis/TypeAnalysis.hpp"

namespace llvm::memoir {

/*
 * AssertStructTypeInst implementation
 */
Type &AssertStructTypeInst::getType() const {
  return *(TypeAnalysis::get().getType(this->getTypeOperand()));
}

llvm::Value &AssertStructTypeInst::getTypeOperand() const {
  return *(this->getTypeOperandAsUse().get());
}

llvm::Use &AssertStructTypeInst::getTypeOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

Struct &AssertStructTypeInst::getStruct() const {
  // TODO: call the StructAnalysis
  return;
}

llvm::Value &AssertStructTypeInst::getStructOperand() const {
  return *(this->getStructOperandAsUse().get());
}

llvm::Use &AssertStructTypeInst::getStructOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(1);
}

std::string AssertStructTypeInst::toString(std::string indent) const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "AssertStructTypeInst: " + llvm_str;

  return str;
}

/*
 * AssertCollectionTypeInst implementation
 */
Type &AssertCollectionTypeInst::getType() const {
  return *(TypeAnalysis::get().getType(this->getTypeOperand()));
}

llvm::Value &AssertCollectionTypeInst::getTypeOperand() const {
  return *(this->getTypeOperandAsUse().get());
}

llvm::Use &AssertCollectionTypeInst::getTypeOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

Collection &AssertCollectionTypeInst::getCollection() const {
  // TODO: call the CollectionAnalysis
  return;
}

llvm::Value &AssertCollectionTypeInst::getCollectionOperand() const {
  return *(this->getCollectionOperandAsUse().get());
}

llvm::Use &AssertCollectionTypeInst::getCollectionOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(1);
}

std::string AssertCollectionTypeInst::toString(std::string indent) const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "AssertCollectionTypeInst: " + llvm_str;

  return str;
}

} // namespace llvm::memoir
