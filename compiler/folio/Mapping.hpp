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
  using LocalsMap = Map<llvm::Value *, llvm::AllocaInst *>;

protected:
  llvm::Value *_alloc;
  GlobalsMap _globals;
  LocalsMap _locals;

public:
  Mapping() : _alloc(NULL), _globals{} {}

  // Allocation accessors.
  llvm::Value &alloc() const;

  void alloc(llvm::Value &V);

  // Globals accessors.
  llvm::GlobalVariable &global(llvm::Value *base) const;
  void global(llvm::Value *base, llvm::GlobalVariable &GV);
  GlobalsMap &globals();
  const GlobalsMap &globals() const;

  // Locals accessors.
  llvm::AllocaInst &local(llvm::Value *base) const;
  void local(llvm::Value *base, llvm::AllocaInst &stack);
  LocalsMap &locals();
  const LocalsMap &locals() const;
};

} // namespace folio

#endif // FOLIO_MAPPING_H
