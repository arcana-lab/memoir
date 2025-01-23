#include "memoir/ir/Instructions.hpp"

#include "memoir/support/Casting.hpp"

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

OPERAND(FoldInst, FunctionOperand, 0)

llvm::Function &FoldInst::getFunction() const {
  auto &F =
      MEMOIR_SANITIZE(dyn_cast<llvm::Function>(&this->getFunctionOperand()),
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
  return MEMOIR_SANITIZE(this->getFunction().getArg(0),
                         "Malformed fold function!");
}

llvm::Argument &FoldInst::getIndexArgument() const {
  return MEMOIR_SANITIZE(this->getFunction().getArg(1),
                         "Malformed fold function!");
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

  auto &arg = MEMOIR_SANITIZE(this->getFunction().getArg(2),
                              "Malformed fold function!");

  return &arg;
}

llvm::Argument &FoldInst::getClosedArgument(llvm::Use &U) const {
  // Get the collection type.
  auto &collection_type =
      MEMOIR_SANITIZE(dyn_cast<CollectionType>(&this->getElementType()),
                      "FoldInst over a non-collection");

  // Fetch the operand number of the closed keyword.
  auto closed_kw = this->getClosed();
  MEMOIR_ASSERT(closed_kw.has_value(),
                "Trying to get closed argument for non-existent closure");
  auto closed_kw_no = closed_kw->getAsUse().getOperandNo();

  // Compute the index of this use within the list of closed arguments.
  auto closed_index = U.getOperandNo() - closed_kw_no - 1;

  // If the element type is void, the first closed argument is at operand 2,
  // otherwise operand 3.
  auto first_closed =
      (isa<VoidType>(&collection_type.getElementType())) ? 2 : 3;

  // Compute the argument number corresponding to the closed operand.
  auto arg_no = closed_index + first_closed;
  auto &arg = MEMOIR_SANITIZE(this->getFunction().getArg(arg_no),
                              "Failed to get function argument.");

  return arg;
}

// llvm::Use &FoldInst::getOperandForArgument(llvm::Argument &A) const {
//   // Get the collection type.
//   auto &collection_type =
//       MEMOIR_SANITIZE(dyn_cast<CollectionType>(&this->getObjectType()),
//                       "FoldInst over a non-collection");
//   auto &element_type = collection_type.getElementType();

//   // If the element type is void, the first closed argument
//   // is at operand 2, otherwise operand 3.
//   auto first_closed =
//       (isa<VoidType>(&collection_type.getElementType())) ? 2 : 3;

//   // Get the argument number.
//   auto arg_no = A.getArgNo();

//   switch (arg_no) {
//     case 0:
//       return this->getInitialAsUse();
//     case 1:
//       return this->getObjectAsUse();
//     case 2:
//       if (not isa<VoidType>(&element_type)) {
//         return this->getObjectAsUse();
//       }
//     default:
//       break;
//   }

//   // Calculate the corresponding operand number.
//   auto closed_no = arg_no - first_closed;

//   return this->getClosedAsUse(closed_no);
// }

TO_STRING(FoldInst)

} // namespace llvm::memoir
