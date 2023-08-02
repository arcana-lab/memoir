#include "memoir/analysis/CollectionAnalysis.hpp"
#include "memoir/ir/Instructions.hpp"
#include "memoir/support/Assert.hpp"

namespace llvm::memoir {

/*
 * AssocRemoveInst implementation.
 */
Collection &AssocRemoveInst::getCollection() const {
  auto collection =
      CollectionAnalysis::analyze(this->getCallInst().getArgOperandUse(0));
  MEMOIR_NULL_CHECK(collection,
                    "Could not determine the sequence after removal");
  return *collection;
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

/*
 * AssocKeysInst implementation.
 */
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
