#include "memoir/ir/Instructions.hpp"

#include "memoir/support/Assert.hpp"

#include "memoir/analysis/CollectionAnalysis.hpp"

namespace llvm::memoir {

Collection &JoinInst::getCollection() const {
  auto collection = CollectionAnalysis::analyze(this->getCallInst());
  MEMOIR_NULL_CHECK(collection,
                    "Could not determine the collection after join");
  return *collection;
}

unsigned JoinInst::getNumberOfJoins() const {
  auto &num_joins_value = this->getNumberOfJoinsOperand();
  auto num_joins_constant = dyn_cast<llvm::ConstantInt>(&num_joins_value);
  MEMOIR_NULL_CHECK(
      num_joins_constant,
      "Attempt to perform join operation with a non-static number of collections");

  auto num_joins = num_joins_constant->getSExtValue();
  MEMOIR_ASSERT(
      (num_joins < 256),
      "Attempt to perform join operation with more than 255 collections.\n"
      "This is unsupported due to llvm::CallInst only accepting 255 arguments.");

  return num_joins;
}

llvm::Value &JoinInst::getNumberOfJoinsOperand() const {
  return *(this->getNumberOfJoinsOperandAsUse().get());
}

llvm::Use &JoinInst::getNumberOfJoinsOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

Collection &JoinInst::getJoinedCollection(unsigned join_index) const {
  auto collection =
      CollectionAnalysis::analyze(this->getJoinedOperandAsUse(join_index));
  MEMOIR_NULL_CHECK(collection,
                    "Could not determine the collection being joined");
  return *collection;
}

llvm::Value &JoinInst::getJoinedOperand(unsigned join_index) const {
  return *(this->getJoinedOperandAsUse(join_index).get());
}

llvm::Use &JoinInst::getJoinedOperandAsUse(unsigned join_index) const {
  return this->getCallInst().getArgOperandUse(1 + join_index);
}

std::string JoinInst::toString(std::string indent) const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "JoinInst: " + llvm_str;

  return str;
}

} // namespace llvm::memoir
