#ifndef FOLIO_TRANSFORMS_CONTEXT_H
#define FOLIO_TRANSFORMS_CONTEXT_H

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/User.h"
#include "llvm/IR/Value.h"

#include "memoir/support/DataTypes.hpp"

namespace memoir {

struct Context {
  llvm::Function &function() const {
    return *this->_func;
  }

  llvm::CallBase *caller() const {
    return this->_call;
  }

  Context(llvm::Function &func, llvm::CallBase *caller = NULL)
    : Context(&func, caller) {}
  Context(llvm::Function &func, llvm::CallBase &caller)
    : Context(&func, &caller) {}
  Context(llvm::Function &func) : Context(&func, NULL) {}

  friend bool operator<(const Context &a, const Context &b) {
    return (a._func < b._func) and (a._call < b._call);
  }

  friend bool operator==(const Context &a, const Context &b) {
    return (a._func == b._func) and (a._call == b._call);
  }

  friend bool operator<(const Context &ctx, const llvm::Function &func) {
    return (ctx._func < &func) and (ctx._call != NULL);
  }

  friend bool operator==(const Context &ctx, const llvm::Function &func) {
    return (ctx._func == &func) and (ctx._call == NULL);
  }

protected:
  Context(llvm::Function *func, llvm::CallBase *caller)
    : _func(func),
      _call(caller) {}

  llvm::Function *_func;
  llvm::CallBase *_call;
};

template <typename T>
using ContextMapBase =
    OrderedMultiMap<llvm::Function *, Pair<llvm::CallBase *, T>>;

template <typename T>
struct ContextMap : public ContextMapBase<T> {
  using Base = ContextMapBase<T>;
  using Iter = Base::iterator;

  using Base::emplace;
  T &emplace(llvm::Function *func, llvm::CallBase *call = NULL) {
    // Try to find an existing item.
    auto [it, ie] = this->Base::equal_range(func);
    for (; it != ie; ++it) {
      auto &[it_func, it_pair] = *it;
      auto &[it_call, it_val] = it_pair;
      if (call == it_call) {
        return it_val;
      }
    }

    // Otherwise, insert.
    it = this->Base::insert(it, make_pair(func, make_pair(call, T())));
    auto &[_func, pair] = *it;
    return pair.second;
  }

  const T &operator[](llvm::Function *func) const {
    // Assume null pair.
    auto [it, ie] = this->Base::equal_range(func);
    for (; it != ie; ++ie) {
      const auto &[call, val] = *it;
      if (call == NULL) {
        return val;
      }
    }
  }
};

} // namespace memoir

template <>
struct std::hash<folio::Context> {
  std::size_t operator()(const folio::Context &C) const noexcept {
    std::size_t h1 = std::hash<llvm::Function *>{}(&C.function());
    std::size_t h2 = std::hash<llvm::CallBase *>{}(C.caller());
    return h1 ^ (h2 << 1); // or use boost::hash_combine
  }
};

#endif // FOLIO_TRANSFORMS_CONTEXT_H
