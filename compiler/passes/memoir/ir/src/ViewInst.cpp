#include "memoir/ir/Instructions.hpp"

#include "memoir/support/Assert.hpp"

#include "memoir/analysis/CollectionAnalysis.hpp"

namespace llvm::memoir {

/*
 * ViewInst implemenatation
 */
Collection &ViewInst::getView() const {
  auto collection = CollectionAnalysis::analyze(this->getViewAsValue());
  MEMOIR_NULL_CHECK(collection, "Could not determine collection view");
  return *collection;
}

llvm::Value &ViewInst::getViewAsValue() const {
  return this->getCallInst();
}

Collection &ViewInst::getCollection() const {
  auto collection =
      CollectionAnalysis::analyze(this->getCollectionOperandAsUse());
  MEMOIR_NULL_CHECK(collection, "Could not determine viewed collection");
  return *collection;
}

llvm::Value &ViewInst::getCollectionOperand() const {
  return *(this->getCollectionOperandAsUse().get());
}

llvm::Use &ViewInst::getCollectionOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

llvm::Value &ViewInst::getBeginIndex() const {
  return *(this->getBeginIndexAsUse().get());
}

llvm::Use &ViewInst::getBeginIndexAsUse() const {
  return this->getCallInst().getArgOperandUse(1);
}

llvm::Value &ViewInst::getEndIndex() const {
  return *(this->getEndIndexAsUse().get());
}

llvm::Use &ViewInst::getEndIndexAsUse() const {
  return this->getCallInst().getArgOperandUse(2);
}

std::string ViewInst::toString(std::string indent) const {
  return "view";
}

} // namespace llvm::memoir
