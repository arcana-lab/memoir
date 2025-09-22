#include "memoir/ir/Instructions.hpp"

#include "memoir/ir/TypeCheck.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/utility/InstructionUtils.hpp"

namespace memoir {

// AllocInst implementation
RESULTANT(AllocInst, Allocation)

Type &AllocInst::getType() const {
  return MEMOIR_SANITIZE(type_of(this->getTypeOperand()),
                         "Could not determine allocation type");
}

OPERAND(AllocInst, TypeOperand, 0)

llvm::iterator_range<AllocInst::size_iterator> AllocInst::sizes() {
  return llvm::make_range(this->sizes_begin(), this->sizes_end());
}

AllocInst::size_iterator AllocInst::sizes_begin() {
  return size_iterator(this->size_ops_begin());
}

AllocInst::size_iterator AllocInst::sizes_end() {
  return size_iterator(this->size_ops_end());
}

llvm::iterator_range<AllocInst::const_size_iterator> AllocInst::sizes() const {
  return llvm::make_range(this->sizes_begin(), this->sizes_end());
}

AllocInst::const_size_iterator AllocInst::sizes_begin() const {
  return const_size_iterator(this->size_ops_begin());
}

AllocInst::const_size_iterator AllocInst::sizes_end() const {
  return const_size_iterator(this->size_ops_end());
}

llvm::iterator_range<AllocInst::size_op_iterator> AllocInst::size_operands() {
  return llvm::make_range(this->size_ops_begin(), this->size_ops_end());
}

AllocInst::size_op_iterator AllocInst::size_ops_begin() {
  return size_op_iterator(std::next(&this->getTypeOperandAsUse()));
}

AllocInst::size_op_iterator AllocInst::size_ops_end() {
  return size_op_iterator(this->kw_begin().asUse());
}

llvm::iterator_range<AllocInst::const_size_op_iterator> AllocInst::
    size_operands() const {
  return llvm::make_range(this->size_ops_begin(), this->size_ops_end());
}

AllocInst::const_size_op_iterator AllocInst::size_ops_begin() const {
  return const_size_op_iterator(std::next(&this->getTypeOperandAsUse()));
}

AllocInst::const_size_op_iterator AllocInst::size_ops_end() const {
  return const_size_op_iterator(this->kw_begin().asUse());
}

std::string AllocInst::toString() const {
  auto &call = this->getCallInst();
  std::string str =
      value_name(call) + " = new " + this->getType().toString() + "(";

  bool first = true;
  for (auto *size : this->sizes()) {
    if (first) {
      first = false;
    } else {
      str += ", ";
    }

    str += value_name(*size);
  }

  str += ")";

  if (const auto &loc = call.getDebugLoc()) {
    str += " @";
    auto *scope = cast<llvm::DIScope>(loc.getScope());
    str += scope->getFilename();
    str += ":" + std::to_string(loc.getLine());
    if (loc.getCol() != 0)
      str += ":" + std::to_string(loc.getCol());
  }

  return str;
}

} // namespace memoir
