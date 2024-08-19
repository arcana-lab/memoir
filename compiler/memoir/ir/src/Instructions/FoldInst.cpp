#include "memoir/ir/Instructions.hpp"

#include "memoir/utility/InstructionUtils.hpp"

namespace llvm::memoir {

// FoldInst implementation.
RESULTANT(FoldInst, Result)

bool FoldInst::isReverse() const {
  switch (this->getKind()) {
#define HANDLE_FOLD_INST(ENUM, FUNC, CLASS, REVERSE)                           \
  case MemOIR_Func::ENUM:                                                      \
    return REVERSE;
#include "memoir/ir/Instructions.def"
    default:
      MEMOIR_UNREACHABLE("Unknown FoldInst enum.");
  }
}

OPERAND(FoldInst, Initial, 0)

OPERAND(FoldInst, Collection, 1)

llvm::Function &FoldInst::getFunction() const {
  auto &F =
      MEMOIR_SANITIZE(dyn_cast<llvm::Function>(&this->getFunctionOperand()),
                      "FoldInst passed an indirect function to call!");

  MEMOIR_ASSERT(not F.empty(), "FoldInst passed an empty function to call!");

  return F;
}

OPERAND(FoldInst, FunctionOperand, 2)

unsigned FoldInst::getNumberOfClosed() const {
  return (this->getCallInst().arg_size() - 3);
}

VAR_OPERAND(FoldInst, Closed, 3)

llvm::Argument &FoldInst::getAccumulatorArgument() const {
  return MEMOIR_SANITIZE(this->getFunction().getArg(0),
                         "Malformed fold function!");
}

llvm::Argument &FoldInst::getIndexArgument() const {
  return MEMOIR_SANITIZE(this->getFunction().getArg(1),
                         "Malformed fold function!");
}

llvm::Argument *FoldInst::getElementArgument() const {

  // Get the collection type.
  auto &collection_type = MEMOIR_SANITIZE(
      dyn_cast_or_null<CollectionType>(type_of(this->getCollection())),
      "FoldInst over a non-collection");

  // If the element type is void, the first closed argument is at operand 2,
  // otherwise operand 3.
  if (isa<VoidType>(&collection_type.getElementType())) {
    return nullptr;
  }

  auto &arg = MEMOIR_SANITIZE(this->getFunction().getArg(2),
                              "Malformed fold function!");

  return &arg;
}

llvm::Argument &FoldInst::getClosedArgument(llvm::Use &U) const {
  // Get the collection type.
  auto &collection_type = MEMOIR_SANITIZE(
      dyn_cast_or_null<CollectionType>(type_of(this->getCollection())),
      "FoldInst over a non-collection");

  // If the element type is void, the first closed argument is at operand 2,
  // otherwise operand 3.
  auto first_closed =
      (isa<VoidType>(&collection_type.getElementType())) ? 2 : 3;

  auto arg_no = U.getOperandNo() - 3 + first_closed;

  auto &arg = MEMOIR_SANITIZE(this->getFunction().getArg(arg_no),
                              "Out-of-range argument number");

  return arg;
}

TO_STRING(FoldInst)

} // namespace llvm::memoir
