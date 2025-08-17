#ifndef FOLIO_MAPPING_H
#define FOLIO_MAPPING_H

#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"

#include "folio/NestedObject.hpp"
#include "folio/ObjectInfo.hpp"
#include "folio/Utilities.hpp"

namespace folio {

/**
 * Mapping contains information about the values where a given mapping can be
 * found.
 */
struct Mapping {
public:
  using GlobalsMap = Map<ObjectInfo *, llvm::GlobalVariable *>;
  using LocalsMap = Map<ObjectInfo *, llvm::AllocaInst *>;

protected:
  llvm::Value *_alloc;
  GlobalsMap _globals;
  LocalsMap _locals;

public:
  Mapping() : _alloc(NULL), _globals{}, _locals{} {}

  // Globals accessors.
  llvm::GlobalVariable &global(ObjectInfo &base) const;
  void global(ObjectInfo &base, llvm::GlobalVariable &GV);
  GlobalsMap &globals();
  const GlobalsMap &globals() const;

  // Locals accessors.
  llvm::AllocaInst &local(ObjectInfo &base) const;
  void local(ObjectInfo &base, llvm::AllocaInst &stack);
  LocalsMap &locals();
  const LocalsMap &locals() const;
};

} // namespace folio

#endif // FOLIO_MAPPING_H
