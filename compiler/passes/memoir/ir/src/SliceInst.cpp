#include "memoir/ir/Instructions.hpp"

#include "memoir/support/Assert.hpp"

#include "memoir/analysis/CollectionAnalysis.hpp"

namespace llvm::memoir {

Collection &SliceInst::getSlice() const {
  auto collection = CollectionAnalysis::analyze(this->getSliceAsValue());
  MEMOIR_NULL_CHECK(collection, "Could not determine the resulting slice");
  return *collection;
}

llvm::Value &SliceInst::getSliceAsValue() const {
  return this->getCallInst();
}

Collection &SliceInst::getCollection() const {
  auto collection =
      CollectionAnalysis::analyze(this->getCollectionOperandAsUse());
  MEMOIR_NULL_CHECK(collection,
                    "Could not determine the collection being sliced");
  return *collection;
}

llvm::Value &SliceInst::getCollectionOperand() const {
  return *(this->getCollectionOperandAsUse().get());
}

llvm::Use &SliceInst::getCollectionOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

llvm::Value &SliceInst::getBeginIndex() const {
  return *(this->getBeginIndexAsUse().get());
}

llvm::Use &SliceInst::getBeginIndexAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

llvm::Value &SliceInst::getEndIndex() const {
  return *(this->getEndIndexAsUse().get());
}

llvm::Use &SliceInst::getEndIndexAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

std::string SliceInst::toString(std::string indent) const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "SliceInst: " + llvm_str;

  return str;
}

} // namespace llvm::memoir
