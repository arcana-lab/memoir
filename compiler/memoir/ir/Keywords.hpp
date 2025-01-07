#ifndef MEMOIR_KEYWORDS_H
#define MEMOIR_KEYWORDS_H

#include <string>
#include <type_traits>

#include "llvm/IR/Constants.h"
#include "llvm/IR/User.h"

#include "memoir/support/Assert.hpp"

namespace llvm::memoir {

struct Keyword {
public:
  using iterator = llvm::User::value_op_iterator;
  using operand_iterator = llvm::User::op_iterator;
  using const_iterator = llvm::User::const_value_op_iterator;
  using const_operand_iterator = llvm::User::const_op_iterator;

  /**
   * Check if the given value is a keyword string.
   */
  static bool is_keyword(const llvm::Value &V);

  /**
   * Get the LLVM constant for the given keyword.
   */
  template <
      typename KeywordTy,
      std::enable_if_t<std::is_base_of_v<Keyword, KeywordTy>, bool> = true>
  static llvm::ConstantDataArray &get_llvm(llvm::LLVMContext &C) {
    std::string name =
        std::string(Keyword::PREFIX) + std::string(KeywordTy::NAME);

    auto *data = llvm::ConstantDataArray::get(C, name);

    return MEMOIR_SANITIZE(dyn_cast_or_null<llvm::ConstantDataArray>(data),
                           "Failed to create keyword as LLVM constant.");
  }

  /**
   * Get the operand use of this keyword.
   */
  llvm::Use &getAsUse();
  const llvm::Use &getAsUse() const;

  llvm::iterator_range<iterator> values();
  iterator begin();
  iterator end();

  llvm::iterator_range<operand_iterator> operands();
  operand_iterator op_begin();
  operand_iterator op_end();

  llvm::iterator_range<const_iterator> values() const;
  const_iterator begin() const;
  const_iterator end() const;

  llvm::iterator_range<const_operand_iterator> operands() const;
  const_operand_iterator op_begin() const;
  const_operand_iterator op_end() const;

  Keyword(const llvm::Use &use) : use(&use) {}

  static const char *PREFIX;

protected:
  const llvm::Use *use;
};

struct keyword_iterator {
public:
  using value_type = Keyword;

  keyword_iterator(llvm::Use *op) : op(op) {}

  value_type operator*() const;

  keyword_iterator &operator++();

  friend bool operator==(const keyword_iterator &lhs,
                         const keyword_iterator &rhs) {
    return lhs.op == rhs.op;
  }

  llvm::Use *asUse() const {
    return this->op;
  }

protected:
  llvm::Use *op;
};

struct ClosedKeyword : public Keyword {
public:
  llvm::iterator_range<Keyword::iterator> args();
  Keyword::iterator args_begin();
  Keyword::iterator args_end();

  llvm::iterator_range<Keyword::operand_iterator> arg_operands();
  Keyword::operand_iterator arg_ops_begin();
  Keyword::operand_iterator arg_ops_end();

  static bool classof(const Keyword &kw) {
    auto *value = kw.getAsUse().get();
    auto *data = cast<llvm::ConstantDataArray>(value);
    auto str = data->getAsCString();
    return str.ends_with(NAME);
  }

  ClosedKeyword(const llvm::Use &use) : Keyword(use) {}

protected:
  static const char *NAME;

  friend struct Keyword;
};

struct InputKeyword : public Keyword {
public:
  llvm::Value &getInput() const;
  llvm::Use &getInputAsUse() const;

  llvm::iterator_range<Keyword::iterator> indices();
  Keyword::iterator indices_begin();
  Keyword::iterator indices_end();

  llvm::iterator_range<Keyword::operand_iterator> index_operands();
  Keyword::operand_iterator index_ops_begin();
  Keyword::operand_iterator index_ops_end();

  static bool classof(const Keyword &kw) {
    auto *value = kw.getAsUse().get();
    auto *data = cast<llvm::ConstantDataArray>(value);
    auto str = data->getAsCString();
    return str.ends_with(NAME);
  }

  InputKeyword(const llvm::Use &use) : Keyword(use) {}

protected:
  static const char *NAME;

  friend struct Keyword;
};

struct RangeKeyword : public Keyword {
public:
  llvm::Value &getBegin() const;
  llvm::Use &getBeginAsUse() const;

  llvm::Value &getEnd() const;
  llvm::Use &getEndAsUse() const;

  static bool classof(const Keyword &kw) {
    auto *value = kw.getAsUse().get();
    auto *data = cast<llvm::ConstantDataArray>(value);
    auto str = data->getAsCString();
    return str.ends_with(NAME);
  }

  RangeKeyword(const llvm::Use &use) : Keyword(use) {}

protected:
  static const char *NAME;

  friend struct Keyword;
};

struct ValueKeyword : public Keyword {
public:
  llvm::Value &getValue() const;
  llvm::Use &getValueAsUse() const;

  static bool classof(const Keyword &kw) {
    auto *value = kw.getAsUse().get();
    auto *data = cast<llvm::ConstantDataArray>(value);
    auto str = data->getAsCString();
    return str.ends_with(NAME);
  }

  ValueKeyword(const llvm::Use &use) : Keyword(use) {}

protected:
  static const char *NAME;

  friend struct Keyword;
};

} // namespace llvm::memoir

#endif // MEMOIR_KEYWORDS_H
