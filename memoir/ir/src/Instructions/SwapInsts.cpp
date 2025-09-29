#if 0
#  include "memoir/utility/InstructionUtils.hpp"

namespace memoir {

  
// Abstract SwapInst implementation.
RESULTANT(SwapInst, Result)

// SeqSwapInst implementation.
OPERAND(SeqSwapInst, FromCollection, 0)
OPERAND(SeqSwapInst, BeginIndex, 1)
OPERAND(SeqSwapInst, EndIndex, 2)
OPERAND(SeqSwapInst, ToCollection, 3)
OPERAND(SeqSwapInst, ToBeginIndex, 4)
TO_STRING(SeqSwapInst, "swap")

llvm::Value &SeqSwapInst::getIncomingCollectionFor(
    llvm::Value &collection) const {
  return *(this->getIncomingCollectionAsUseFor(collection).get());
}

llvm::Use &SeqSwapInst::getIncomingCollectionAsUseFor(
    llvm::Value &collection) const {
  // Determine which incoming collection matches this value from the result
  // pair.
  auto &extract_inst = MEMOIR_SANITIZE(
      dyn_cast<llvm::ExtractValueInst>(&collection),
      "Trying to get original collection for value that is not from an aggregate.");

  // Check that this is extracted from the swap instruction.
  auto *aggregate = extract_inst.getAggregateOperand();
  MEMOIR_ASSERT(
      aggregate == &this->getResult(),
      "Trying to get original collection for pair that is not from this SwapInst!");

  // Get the index of the incoming value.
  MEMOIR_ASSERT(
      extract_inst.getNumIndices() == 1,
      "Malformed aggregate, either the compiler or declation is out of date!");
  auto index = *(extract_inst.idx_begin());

  // Get the correct input for this index.
  switch (index) {
    case 0:
      return this->getFromCollectionAsUse();
    case 1:
      return this->getToCollectionAsUse();
    default:
      MEMOIR_UNREACHABLE(
          "Malformed aggregate index, either the compiler or decl is out of date!");
  }
}

// SeqSwapWithinInst implementation.
OPERAND(SeqSwapWithinInst, FromCollection, 0)
OPERAND(SeqSwapWithinInst, ToCollection, 0)
OPERAND(SeqSwapWithinInst, BeginIndex, 1)
OPERAND(SeqSwapWithinInst, EndIndex, 2)
OPERAND(SeqSwapWithinInst, ToBeginIndex, 3)
TO_STRING(SeqSwapWithinInst, "swap.within")

llvm::Value &SeqSwapWithinInst::getIncomingCollectionFor(
    llvm::Value &collection) const {
  return *(this->getIncomingCollectionAsUseFor(collection).get());
}

llvm::Use &SeqSwapWithinInst::getIncomingCollectionAsUseFor(
    llvm::Value &collection) const {
  // Determine if the collection value is the same as our result.
  MEMOIR_ASSERT(&collection == &this->getResult(),
                "Value being checked is not the result of this SwapInst!");

  return this->getFromCollectionAsUse();
}

} // namespace memoir
#endif
