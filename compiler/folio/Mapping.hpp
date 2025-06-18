#ifndef FOLIO_MAPPING_H
#define FOLIO_MAPPING_H

#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"

#include "folio/NestedObject.hpp"
#include "folio/Utilities.hpp"

namespace folio {

/**
 * Mapping contains information about the values where a given mapping can be
 * found.
 */
struct Mapping {
public:
  using GlobalsMap = Map<llvm::Value *, llvm::GlobalVariable *>;
  using LocalsMap = Map<llvm::Function *, llvm::AllocaInst *>;

protected:
  llvm::Value *_alloc;
  GlobalsMap _globals;
  LocalsMap _locals;

public:
  Mapping() : _alloc(NULL), _globals{}, _locals{} {}

  // Allocation accessors.
  llvm::Value &alloc() const {
    return *this->_alloc;
  }

  void alloc(llvm::Value &V) {
    this->_alloc = &V;
  }

  // Globals accessors.
  llvm::GlobalVariable &global(llvm::Value *base) const {
    return *this->_globals.at(base);
  }

  void global(llvm::Value *base, llvm::GlobalVariable &GV) {
    this->_globals[base] = &GV;
  }

  GlobalsMap &globals() {
    return this->_globals;
  }

  const GlobalsMap &globals() const {
    return this->_globals;
  }

  // Locals accessors.
  llvm::AllocaInst *local(llvm::Function &F) const {
    auto found = this->_locals.find(&F);
    if (found == this->_locals.end()) {
      return NULL;
    }

    return found->second;
  }

  void local(llvm::Function &func, llvm::AllocaInst &alloca) {
    this->_locals[&func] = &alloca;
  }
};

} // namespace folio

#endif // FOLIO_MAPPING_H
