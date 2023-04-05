#include "memoir/ir/Instructions.hpp"

#include "memoir/support/Assert.hpp"

#include "memoir/analysis/CollectionAnalysis.hpp"

namespace llvm::memoir {

llvm::Value &SizeInst::getSize() const {
  return this->getCallInst();
}

Collection &SizeInst::getCollection() const {
  auto collection =
      CollectionAnalysis::analyze(this->getCollectionOperandAsUse());
  MEMOIR_NULL_CHECK(collection,
                    "Could not determine the collection being sized");
  return *collection;
}

llvm::Value &SizeInst::getCollectionOperand() const {
  return *(this->getCollectionOperandAsUse().get());
}

llvm::Use &SizeInst::getCollectionOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

std::string SizeInst::toString(std::string indent) const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "SizeInst: " + llvm_str;

  return str;
}

} // namespace llvm::memoir
