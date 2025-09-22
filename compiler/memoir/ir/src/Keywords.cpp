#include "memoir/ir/Keywords.hpp"
#include "memoir/support/Assert.hpp"
#include "memoir/utility/FunctionNames.hpp"

using namespace llvm::memoir;

const char *Keyword::PREFIX = "memoir.";
#define KEYWORD(STR, CLASS) const char *CLASS::NAME = #STR;
#include "memoir/ir/Keywords.def"

bool Keyword::is_keyword(llvm::Value *value) {
  if (value == nullptr) {
    return false;
  }
  return Keyword::is_keyword(*value);
}

bool Keyword::is_keyword(llvm::Value &V) {
  llvm::ConstantDataSequential *data = nullptr;
  if (auto *global = dyn_cast<llvm::GlobalVariable>(&V)) {
    auto *init = global->getInitializer();
    data = dyn_cast_or_null<llvm::ConstantDataSequential>(init);
  } else {
    data = dyn_cast<llvm::ConstantDataSequential>(&V);
  }

  if (not data) {
    return false;
  }

  if (not data->isCString()) {
    return false;
  }
  auto str = data->getAsCString();
  if (str.starts_with(Keyword::PREFIX)) {
    return true;
  }
  return false;
}

namespace llvm::memoir::detail {

llvm::Use *find_end_of_keyword(llvm::Use &U) {
  // Find either the next keyword or the end of the operand list.
  auto *call = dyn_cast<llvm::CallBase>(U.getUser());
  auto *curr = std::next(&U);
  while (curr != call->arg_end()) {
    if (Keyword::is_keyword(curr->get())) {
      return curr;
    }
    curr = std::next(curr);
  }

  return curr;
}

} // namespace llvm::memoir::detail

llvm::Use &Keyword::getAsUse() const {
  return *this->use;
}

llvm::iterator_range<Keyword::iterator> Keyword::values() {
  return llvm::make_range(this->begin(), this->end());
}

Keyword::iterator Keyword::begin() {
  return iterator(this->op_begin());
}

Keyword::iterator Keyword::end() {
  return iterator(this->op_end());
}

llvm::iterator_range<Keyword::operand_iterator> Keyword::operands() {
  return llvm::make_range(this->op_begin(), this->op_end());
}

Keyword::operand_iterator Keyword::op_begin() {
  return Keyword::operand_iterator(&this->getAsUse());
}

Keyword::operand_iterator Keyword::op_end() {
  return Keyword::operand_iterator(
      detail::find_end_of_keyword(this->getAsUse()));
}

llvm::iterator_range<Keyword::const_iterator> Keyword::values() const {
  return llvm::make_range(this->begin(), this->end());
}

Keyword::const_iterator Keyword::begin() const {
  return Keyword::const_iterator(this->op_begin());
}

Keyword::const_iterator Keyword::end() const {
  return Keyword::const_iterator(this->op_end());
}

llvm::iterator_range<Keyword::const_operand_iterator> Keyword::operands()
    const {
  return llvm::make_range(this->op_begin(), this->op_end());
}

Keyword::const_operand_iterator Keyword::op_begin() const {
  return Keyword::const_operand_iterator(&this->getAsUse());
}

Keyword::const_operand_iterator Keyword::op_end() const {
  return Keyword::const_operand_iterator(
      detail::find_end_of_keyword(this->getAsUse()));
}

// keyword_iterator implementation
keyword_iterator::value_type keyword_iterator::operator*() const {
  return value_type(*this->op);
}

keyword_iterator &keyword_iterator::operator++() {
  this->op = detail::find_end_of_keyword(*this->op);
  return *this;
}

// ClosedKeyword implementation
llvm::iterator_range<Keyword::iterator> ClosedKeyword::args() {
  return llvm::make_range(this->args_begin(), this->args_end());
}

Keyword::iterator ClosedKeyword::args_begin() {
  return iterator(std::next(&this->getAsUse()));
}

Keyword::iterator ClosedKeyword::args_end() {
  return iterator(this->end());
}

llvm::iterator_range<Keyword::operand_iterator> ClosedKeyword::arg_operands() {
  return llvm::make_range(this->arg_ops_begin(), this->arg_ops_end());
}

Keyword::operand_iterator ClosedKeyword::arg_ops_begin() {
  return operand_iterator(std::next(&this->getAsUse()));
}

Keyword::operand_iterator ClosedKeyword::arg_ops_end() {
  return operand_iterator(this->op_end());
}

// InputKeyword implementation
llvm::Value &InputKeyword::getInput() const {
  return *this->getInputAsUse().get();
}

llvm::Use &InputKeyword::getInputAsUse() const {
  return *std::next(&this->getAsUse());
}

llvm::iterator_range<Keyword::iterator> InputKeyword::indices() {
  return llvm::make_range(this->indices_begin(), this->indices_end());
}

Keyword::iterator InputKeyword::indices_begin() {
  return iterator(std::next(&this->getInputAsUse()));
}

Keyword::iterator InputKeyword::indices_end() {
  return iterator(this->end());
}

llvm::iterator_range<Keyword::operand_iterator> InputKeyword::index_operands() {
  return llvm::make_range(this->index_ops_begin(), this->index_ops_end());
}

Keyword::operand_iterator InputKeyword::index_ops_begin() {
  return operand_iterator(std::next(&this->getInputAsUse()));
}

Keyword::operand_iterator InputKeyword::index_ops_end() {
  return operand_iterator(this->op_end());
}

// RangeKeyword implementation
llvm::Value &RangeKeyword::getBegin() const {
  return *this->getBeginAsUse().get();
}

llvm::Use &RangeKeyword::getBeginAsUse() const {
  return *std::next(&this->getAsUse());
}

llvm::Value &RangeKeyword::getEnd() const {
  return *this->getEndAsUse().get();
}

llvm::Use &RangeKeyword::getEndAsUse() const {
  return *std::next(&this->getBeginAsUse());
}

// ValueKeyword implementation
llvm::Value &ValueKeyword::getValue() const {
  return *this->getValueAsUse().get();
}

llvm::Use &ValueKeyword::getValueAsUse() const {
  return *std::next(&this->getAsUse());
}

// SelectionKeyword implementation
std::string SelectionKeyword::getSelection() const {
  auto &V = this->getSelectionOperand();

  auto *data = dyn_cast<llvm::ConstantDataSequential>(&V);

  if (not data) {
    if (auto *global = dyn_cast<llvm::GlobalVariable>(&V)) {
      auto *init = global->getInitializer();
      data = dyn_cast_or_null<llvm::ConstantDataSequential>(init);
    }
  }

  if (not data or not data->isCString()) {
    MEMOIR_UNREACHABLE(
        "Unhandled LLVM value for selection in SelectionKeyword");
  }

  return data->getAsCString().str();
}

llvm::Value &SelectionKeyword::getSelectionOperand() const {
  return *this->getSelectionOperandAsUse().get();
}

llvm::Use &SelectionKeyword::getSelectionOperandAsUse() const {
  return *std::next(&this->getAsUse());
}

// ADENoShareKeyword implementation
llvm::iterator_range<Keyword::iterator> ADENoShareKeyword::indices() {
  return llvm::make_range(this->indices_begin(), this->indices_end());
}

Keyword::iterator ADENoShareKeyword::indices_begin() {
  return iterator(std::next(&this->getAsUse()));
}

Keyword::iterator ADENoShareKeyword::indices_end() {
  return iterator(this->end());
}

llvm::iterator_range<Keyword::operand_iterator> ADENoShareKeyword::
    index_operands() {
  return llvm::make_range(this->index_ops_begin(), this->index_ops_end());
}

Keyword::operand_iterator ADENoShareKeyword::index_ops_begin() {
  return operand_iterator(std::next(&this->getAsUse()));
}

Keyword::operand_iterator ADENoShareKeyword::index_ops_end() {
  return operand_iterator(this->op_end());
}
