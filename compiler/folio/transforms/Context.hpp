#ifndef FOLIO_TRANSFORMS_CONTEXT_H
#define FOLIO_TRANSFORMS_CONTEXT_H

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/User.h"
#include "llvm/IR/Value.h"

namespace folio {

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

} // namespace folio

template <>
struct std::hash<folio::Context> {
  std::size_t operator()(const folio::Context &C) const noexcept {
    std::size_t h1 = std::hash<llvm::Function *>{}(&C.function());
    std::size_t h2 = std::hash<llvm::CallBase *>{}(C.caller());
    return h1 ^ (h2 << 1); // or use boost::hash_combine
  }
};

#endif // FOLIO_TRANSFORMS_CONTEXT_H
