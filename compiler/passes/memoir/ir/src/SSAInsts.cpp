#include "memoir/ir/Instructions.hpp"

#include "memoir/support/Assert.hpp"

#include "memoir/utility/Metadata.hpp"

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
  // Look for metadata attached to this use.
  auto *value =
      MetadataManager::getMetadata(this->getCallInst(), MetadataType::USE_PHI);
  auto *inst = dyn_cast_or_null<llvm::Instruction>(value);
  MEMOIR_NULL_CHECK(inst, "Couldn't get the UseInst");
  return *inst;
}

void UsePHIInst::setUseInst(llvm::Instruction &I) const {
  // Update this information in the MetadataManager
  MetadataManager::setMetadata(this->getCallInst(), MetadataType::USE_PHI, &I);
}

void UsePHIInst::setUseInst(MemOIRInst &I) const {
  this->setUseInst(I.getCallInst());
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
  // Look for metadata attached to this definition.
  auto *value =
      MetadataManager::getMetadata(this->getCallInst(), MetadataType::DEF_PHI);
  auto *inst = dyn_cast_or_null<llvm::Instruction>(value);
  MEMOIR_NULL_CHECK(inst, "Couldn't get the DefInst");
  return *inst;
}

void DefPHIInst::setDefInst(llvm::Instruction &I) const {
  // Update this information in the MetadataManager
  MetadataManager::setMetadata(this->getCallInst(), MetadataType::DEF_PHI, &I);
}

void DefPHIInst::setDefInst(MemOIRInst &I) const {
  this->setDefInst(I.getCallInst());
}

std::string DefPHIInst::toString(std::string indent) const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "DefPHI: " + llvm_str;

  return str;
}
} // namespace llvm::memoir
