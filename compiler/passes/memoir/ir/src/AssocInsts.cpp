#include "memoir/analysis/CollectionAnalysis.hpp"
#include "memoir/ir/Instructions.hpp"
#include "memoir/support/Assert.hpp"

namespace llvm::memoir {

// AssocRemoveInst implementation.
Collection &AssocRemoveInst::getResultCollection() const {
  return MEMOIR_SANITIZE(
      CollectionAnalysis::analyze(this->getResultAsValue()),
      "Could not determine resultant collection of AssocRemoveInst");
}

llvm::Value &AssocRemoveInst::getResultAsValue() const {
  return this->getCallInst();
}

Collection &AssocRemoveInst::getCollection() const {
  auto collection =
      CollectionAnalysis::analyze(this->getCollectionOperandAsUse());
  MEMOIR_NULL_CHECK(collection,
                    "Could not determine the sequence after removal");
  return *collection;
}

llvm::Value &AssocRemoveInst::getCollectionOperand() const {
  return *(this->getCollectionOperandAsUse().get());
}

llvm::Use &AssocRemoveInst::getCollectionOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

llvm::Value &AssocRemoveInst::getKeyOperand() const {
  return *(this->getKeyOperandAsUse().get());
}

llvm::Use &AssocRemoveInst::getKeyOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(1);
}

std::string AssocRemoveInst::toString(std::string indent) const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "AssocRemove: " + llvm_str;

  return str;
}

// AssocInsertInst implementation.
Collection &AssocInsertInst::getResultCollection() const {
  return MEMOIR_SANITIZE(
      CollectionAnalysis::analyze(this->getResultAsValue()),
      "Could not determine resultant collection of AssocInsertInst");
}

llvm::Value &AssocInsertInst::getResultAsValue() const {
  return this->getCallInst();
}

Collection &AssocInsertInst::getCollection() const {
  return MEMOIR_SANITIZE(
      CollectionAnalysis::analyze(this->getCollectionOperandAsUse()),
      "Could not determine the sequence after removal");
}

llvm::Value &AssocInsertInst::getCollectionOperand() const {
  return *(this->getCollectionOperandAsUse().get());
}

llvm::Use &AssocInsertInst::getCollectionOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

llvm::Value &AssocInsertInst::getKeyOperand() const {
  return *(this->getKeyOperandAsUse().get());
}

llvm::Use &AssocInsertInst::getKeyOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(1);
}

std::string AssocInsertInst::toString(std::string indent) const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "AssocInsert: " + llvm_str;

  return str;
}

// AssocKeysInst implementation.
Collection &AssocKeysInst::getKeys() const {
  auto collection = CollectionAnalysis::analyze(this->getCallInst());
  MEMOIR_NULL_CHECK(collection, "Could not determine the keys of assoc");
  return *collection;
}

Collection &AssocKeysInst::getCollection() const {
  auto collection =
      CollectionAnalysis::analyze(this->getCallInst().getArgOperandUse(0));
  MEMOIR_NULL_CHECK(collection, "Could not determine the assoc for keys");
  return *collection;
}

std::string AssocKeysInst::toString(std::string indent) const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "AssocKeys: " + llvm_str;

  return str;
}

} // namespace llvm::memoir
