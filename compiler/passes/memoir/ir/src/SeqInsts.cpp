#include "memoir/ir/Instructions.hpp"

#include "memoir/support/Assert.hpp"

#include "memoir/analysis/CollectionAnalysis.hpp"

namespace llvm::memoir {

/*
 * SeqInsertInst implementation
 */
Collection &SeqInsertInst::getCollection() const {
  auto collection =
      CollectionAnalysis::analyze(this->getCollectionOperandAsUse());
  MEMOIR_NULL_CHECK(collection,
                    "Could not determine collection being inserted into");
  return *collection;
}

llvm::Value &SeqInsertInst::getCollectionOperand() const {
  return *(this->getCollectionOperandAsUse().get());
}

llvm::Use &SeqInsertInst::getCollectionOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(1);
}

llvm::Value &SeqInsertInst::getValueInserted() const {
  return *(this->getValueInsertedAsUse().get());
}

llvm::Use &SeqInsertInst::getValueInsertedAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

llvm::Value &SeqInsertInst::getIndex() const {
  return *(this->getIndexAsUse().get());
}

llvm::Use &SeqInsertInst::getIndexAsUse() const {
  return this->getCallInst().getArgOperandUse(2);
}

std::string SeqInsertInst::toString(std::string indent) const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "Sequence Insert: " + llvm_str;

  return str;
}

/*
 * SeqRemoveInst implementation.
 */
Collection &SeqRemoveInst::getCollection() const {
  auto collection =
      CollectionAnalysis::analyze(this->getCollectionOperandAsUse());
  MEMOIR_NULL_CHECK(collection,
                    "Could not determine collection being removeed into");
  return *collection;
}

llvm::Value &SeqRemoveInst::getCollectionOperand() const {
  return *(this->getCollectionOperandAsUse().get());
}

llvm::Use &SeqRemoveInst::getCollectionOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

llvm::Value &SeqRemoveInst::getBeginIndex() const {
  return *(this->getBeginIndexAsUse().get());
}

llvm::Use &SeqRemoveInst::getBeginIndexAsUse() const {
  return this->getCallInst().getArgOperandUse(1);
}

llvm::Value &SeqRemoveInst::getEndIndex() const {
  return *(this->getEndIndexAsUse().get());
}

llvm::Use &SeqRemoveInst::getEndIndexAsUse() const {
  return this->getCallInst().getArgOperandUse(2);
}

std::string SeqRemoveInst::toString(std::string indent) const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "SeqRemove: " + llvm_str;

  return str;
}

/*
 * SeqAppendInst implementation.
 */
Collection &SeqAppendInst::getCollection() const {
  auto collection =
      CollectionAnalysis::analyze(this->getCollectionOperandAsUse());
  MEMOIR_NULL_CHECK(collection,
                    "Could not determine collection being appended into");
  return *collection;
}

llvm::Value &SeqAppendInst::getCollectionOperand() const {
  return *(this->getCollectionOperandAsUse().get());
}

llvm::Use &SeqAppendInst::getCollectionOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

Collection &SeqAppendInst::getAppendedCollection() const {
  auto collection =
      CollectionAnalysis::analyze(this->getAppendedCollectionOperandAsUse());
  MEMOIR_NULL_CHECK(collection,
                    "Could not determine the sequence being appended");
  return *collection;
}

llvm::Value &SeqAppendInst::getAppendedCollectionOperand() const {
  return *(this->getAppendedCollectionOperandAsUse().get());
}

llvm::Use &SeqAppendInst::getAppendedCollectionOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(1);
}

std::string SeqAppendInst::toString(std::string indent) const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "SeqAppendInst: " + llvm_str;

  return str;
}

/*
 * SeqSwapInst implementation.
 */
Collection &SeqSwapInst::getFromCollection() const {
  auto collection =
      CollectionAnalysis::analyze(this->getFromCollectionOperandAsUse());
  MEMOIR_NULL_CHECK(collection, "Could not determine the from sequence");
  return *collection;
}

llvm::Value &SeqSwapInst::getFromCollectionOperand() const {
  return *(this->getFromCollectionOperandAsUse().get());
}

llvm::Use &SeqSwapInst::getFromCollectionOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

llvm::Value &SeqSwapInst::getBeginIndex() const {
  return *(this->getBeginIndexAsUse().get());
}

llvm::Use &SeqSwapInst::getBeginIndexAsUse() const {
  return this->getCallInst().getArgOperandUse(1);
}

llvm::Value &SeqSwapInst::getEndIndex() const {
  return *(this->getEndIndexAsUse().get());
}

llvm::Use &SeqSwapInst::getEndIndexAsUse() const {
  return this->getCallInst().getArgOperandUse(2);
}

Collection &SeqSwapInst::getToCollection() const {
  auto collection =
      CollectionAnalysis::analyze(this->getToCollectionOperandAsUse());
  MEMOIR_NULL_CHECK(collection, "Could not determine the to sequence");
  return *collection;
}

llvm::Value &SeqSwapInst::getToCollectionOperand() const {
  return *(this->getToCollectionOperandAsUse().get());
}

llvm::Use &SeqSwapInst::getToCollectionOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(3);
}

llvm::Value &SeqSwapInst::getToBeginIndex() const {
  return *(this->getToBeginIndexAsUse().get());
}

llvm::Use &SeqSwapInst::getToBeginIndexAsUse() const {
  return this->getCallInst().getArgOperandUse(4);
}

std::string SeqSwapInst::toString(std::string indent) const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "SeqSwap: " + llvm_str;

  return str;
}

/*
 * SeqSplitInst implementation.
 */
Collection &SeqSplitInst::getSplit() const {
  auto collection = CollectionAnalysis::analyze(this->getCallInst());
  MEMOIR_NULL_CHECK(collection, "Could not determine the split sequence");
  return *collection;
}

llvm::Value &SeqSplitInst::getSplitValue() const {
  return this->getCallInst();
}

Collection &SeqSplitInst::getCollection() const {
  auto collection =
      CollectionAnalysis::analyze(this->getCollectionOperandAsUse());
  MEMOIR_NULL_CHECK(collection, "Could not determine the sequence being split");
  return *collection;
}

llvm::Value &SeqSplitInst::getCollectionOperand() const {
  return *(this->getCollectionOperandAsUse().get());
}

llvm::Use &SeqSplitInst::getCollectionOperandAsUse() const {
  return this->getCallInst().getArgOperandUse(0);
}

llvm::Value &SeqSplitInst::getBeginIndex() const {
  return *(this->getBeginIndexAsUse().get());
}

llvm::Use &SeqSplitInst::getBeginIndexAsUse() const {
  return this->getCallInst().getArgOperandUse(1);
}

llvm::Value &SeqSplitInst::getEndIndex() const {
  return *(this->getEndIndexAsUse().get());
}

llvm::Use &SeqSplitInst::getEndIndexAsUse() const {
  return this->getCallInst().getArgOperandUse(2);
}

std::string SeqSplitInst::toString(std::string indent) const {
  std::string str, llvm_str;
  llvm::raw_string_ostream llvm_ss(llvm_str);
  llvm_ss << this->getCallInst();

  str = "SeqSplit: " + llvm_str;

  return str;
}

} // namespace llvm::memoir
