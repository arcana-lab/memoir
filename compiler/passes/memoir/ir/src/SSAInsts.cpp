#include "memoir/ir/Instructions.hpp"

#include "memoir/support/Assert.hpp"

#include "memoir/analysis/CollectionAnalysis.hpp"

namespace llvm::memoir {

/*
 * UsePHIInst implementation
 */
Collection &UsePHIInst::getCollection() const {
  auto collection = CollectionAnalysis::analyze(this->getCollectionValue());
  MEMOIR_NULL_CHECK(collection, "Could not determine result of UsePHI");
  return *collection;
}

llvm::Value &UsePHIInst::getCollectionValue() const {
  return this->getCallInst();
}

Collection &UsePHIInst::getUsedCollection() const {
  auto collection =
      CollectionAnalysis::analyze(this->getUsedCollectionOperandAsUse());
  MEMOIR_NULL_CHECK(collection, "Could not determine collection being used");
  return *collection;
}

llvm::Value &UsePHIInst::getUsedCollectionOperand() const {
  return *(this->getUsedCollectionOperandAsUse().get());
}

llvm::Use &UsePHIInst::getUsedCollectionOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

llvm::Instruction &UsePHIInst::getUseInst() const {
  // TODO: get this information from the MetadataManager
  return this->getCallInst();
}

void UsePHIInst::setUseInst(llvm::Instruction &I) const {
  // TODO: update this information in the MetadataManager
  return;
}

std::string UsePHIInst::toString(std::string indent) const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "UsePHI: " + llvm_str;

  return str;
}

/*
 * DefPHIInst implementation
 */
Collection &DefPHIInst::getCollection() const {
  auto collection = CollectionAnalysis::analyze(this->getCollectionValue());
  MEMOIR_NULL_CHECK(collection, "Could not determine result of DefPHI");
  return *collection;
}

llvm::Value &DefPHIInst::getCollectionValue() const {
  return this->getCallInst();
}

Collection &DefPHIInst::getDefinedCollection() const {
  auto collection =
      CollectionAnalysis::analyze(this->getDefinedCollectionOperandAsUse());
  MEMOIR_NULL_CHECK(collection, "Could not determine collection being used");
  return *collection;
}

llvm::Value &DefPHIInst::getDefinedCollectionOperand() const {
  return *(this->getDefinedCollectionOperandAsUse().get());
}

llvm::Use &DefPHIInst::getDefinedCollectionOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

llvm::Instruction &DefPHIInst::getDefInst() const {
  // TODO: look for metadata attached to this definition.
  return this->getCallInst();
}

void DefPHIInst::setDefInst(llvm::Instruction &I) const {
  // TODO: update this information in the MetadataManager
  return;
}

std::string DefPHIInst::toString(std::string indent) const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "DefPHI: " + llvm_str;

  return str;
}
} // namespace llvm::memoir
