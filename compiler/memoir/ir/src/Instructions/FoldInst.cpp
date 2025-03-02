#include "memoir/ir/Instructions.hpp"

#include "memoir/support/Casting.hpp"

#include "memoir/utility/InstructionUtils.hpp"

namespace llvm::memoir {

// FoldInst implementation.

FoldInst *FoldInst::get_single_fold(llvm::Function &F) {
  FoldInst *fold = nullptr;
  for (auto &use : F.uses()) {
    auto *user = use.getUser();
    auto *call = dyn_cast_or_null<llvm::Instruction>(user);

    // The function may be called indirectly!
    if (not call) {
      return nullptr;
    }

    auto *memoir = into<MemOIRInst>(call);

    // Skip 'fake' uses of the function by RetPHI.
    if (isa<RetPHIInst>(memoir)) {
      continue;
    }

    fold = dyn_cast_or_null<FoldInst>(memoir);

    // Found a non-fold user!
    if (not fold) {
      return nullptr;
    }
  }

  return fold;
}

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

OPERAND(FoldInst, BodyOperand, 0)

llvm::Function &FoldInst::getBody() const {
  auto &F = MEMOIR_SANITIZE(dyn_cast<llvm::Function>(&this->getBodyOperand()),
                            "FoldInst passed an indirect function to call!");

  MEMOIR_ASSERT(not F.empty(), "FoldInst passed an empty function to call!");

  return F;
}

OPERAND(FoldInst, Initial, 1)
OPERAND(FoldInst, Object, 2)

std::optional<ClosedKeyword> FoldInst::getClosed() const {
  for (auto keyword : this->keywords()) {
    auto closed = try_cast<ClosedKeyword>(keyword);
    if (closed) {
      return closed;
    }
  }

  return {};
}

llvm::iterator_range<FoldInst::iterator> FoldInst::closed() {
  return llvm::make_range(this->closed_begin(), this->closed_end());
}
FoldInst::iterator FoldInst::closed_begin() {
  return iterator(this->closed_ops_begin());
}
FoldInst::iterator FoldInst::closed_end() {
  return iterator(this->closed_ops_end());
}

llvm::iterator_range<FoldInst::operand_iterator> FoldInst::closed_operands() {
  return llvm::make_range(this->closed_ops_begin(), this->closed_ops_end());
}
FoldInst::operand_iterator FoldInst::closed_ops_begin() {
  if (auto closed_kw = this->getClosed()) {
    return closed_kw->arg_ops_begin();
  } else {
    return operand_iterator(nullptr);
  }
}
FoldInst::operand_iterator FoldInst::closed_ops_end() {
  if (auto closed_kw = this->getClosed()) {
    return closed_kw->arg_ops_end();
  } else {
    return operand_iterator(nullptr);
  }
}

llvm::iterator_range<FoldInst::const_iterator> FoldInst::closed() const {
  return llvm::make_range(this->closed_begin(), this->closed_end());
}
FoldInst::const_iterator FoldInst::closed_begin() const {
  return const_iterator(this->closed_ops_begin());
}
FoldInst::const_iterator FoldInst::closed_end() const {
  return const_iterator(this->closed_ops_end());
}

llvm::iterator_range<FoldInst::const_operand_iterator> FoldInst::
    closed_operands() const {
  return llvm::make_range(this->closed_ops_begin(), this->closed_ops_end());
}
FoldInst::const_operand_iterator FoldInst::closed_ops_begin() const {
  if (auto closed_kw = this->getClosed()) {
    return closed_kw->arg_ops_begin();
  } else {
    return const_operand_iterator(nullptr);
  }
}
FoldInst::const_operand_iterator FoldInst::closed_ops_end() const {
  if (auto closed_kw = this->getClosed()) {
    return closed_kw->arg_ops_end();
  } else {
    return const_operand_iterator(nullptr);
  }
}

llvm::Argument &FoldInst::getAccumulatorArgument() const {
  return MEMOIR_SANITIZE(this->getBody().getArg(0), "Malformed fold function!");
}

llvm::Argument &FoldInst::getIndexArgument() const {
  return MEMOIR_SANITIZE(this->getBody().getArg(1), "Malformed fold function!");
}

llvm::Argument *FoldInst::getElementArgument() const {

  // Get the collection type.
  auto *type = type_of(this->getObject());
  MEMOIR_ASSERT(type, "Failed to get type of object being folded over.")

  for (auto *index : this->indices()) {
    if (auto *struct_type = dyn_cast<StructType>(type)) {
      auto &index_constant =
          MEMOIR_SANITIZE(dyn_cast<llvm::ConstantInt>(index),
                          "Struct field index is not constant.");
      auto index_value = index_constant.getZExtValue();
      type = &struct_type->getFieldType(index_value);

    } else if (auto *collection_type = dyn_cast<CollectionType>(type)) {
      type = &collection_type->getElementType();
    }
  }

  auto &collection_type =
      MEMOIR_SANITIZE(dyn_cast<CollectionType>(type),
                      "FoldInst is only supported for collection types.");

  // If the element type is void, the first closed argument is at operand 2,
  // otherwise operand 3.
  if (isa<VoidType>(&collection_type.getElementType())) {
    return nullptr;
  }

  auto &arg =
      MEMOIR_SANITIZE(this->getBody().getArg(2), "Malformed fold function!");

  return &arg;
}

llvm::Argument *FoldInst::getClosedArgument(llvm::Use &U) const {
  // Get the collection type.
  auto &collection_type =
      MEMOIR_SANITIZE(dyn_cast<CollectionType>(&this->getElementType()),
                      "FoldInst over a non-collection");

  // Fetch the operand number of the closed keyword.
  auto closed_kw = this->getClosed();
  if (not closed_kw) {
    return nullptr;
  }

  auto closed_kw_no = closed_kw->getAsUse().getOperandNo();
  if (closed_kw_no >= U.getOperandNo()) {
    return nullptr;
  }

  // Compute the index of this use within the list of closed arguments.
  auto closed_index = U.getOperandNo() - closed_kw_no - 1;

  // If the element type is void, the first closed argument is at operand 2,
  // otherwise operand 3.
  auto first_closed =
      (isa<VoidType>(&collection_type.getElementType())) ? 2 : 3;

  // Compute the argument number corresponding to the closed operand.
  auto arg_no = closed_index + first_closed;
  auto *arg = this->getBody().getArg(arg_no);

  return arg;
}

llvm::Use *FoldInst::getOperandForArgument(llvm::Argument &A) const {
  // Get the collection type.
  auto &collection_type =
      MEMOIR_SANITIZE(dyn_cast<CollectionType>(&this->getObjectType()),
                      "FoldInst over a non-collection");
  auto &element_type = collection_type.getElementType();

  if (&A == &this->getAccumulatorArgument()) {
    return &this->getInitialAsUse();
  }

  // Fetch the operand number of the closed keyword.
  if (auto closed_kw = this->getClosed()) {
    for (auto &closed_op : closed_kw->arg_operands()) {
      if (&A == this->getClosedArgument(closed_op)) {
        return &closed_op;
      }
    }
  }

  return nullptr;
}

TO_STRING(FoldInst)

} // namespace llvm::memoir
