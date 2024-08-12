#ifndef MEMOIR_ANALYSIS_DEFUSECHAIN_H
#define MEMOIR_ANALYSIS_DEFUSECHAIN_H

#include "memoir/passes/Passes.hpp"

#include "memoir/ir/Instructions.hpp"

#include "memoir/support/Casting.hpp"
#include "memoir/support/InternalDatatypes.hpp"
#include "memoir/support/UnionFind.hpp"

namespace llvm::memoir {

struct DefUseChainResult {
public:
  /**
   * Query the set of declarations for the given value.
   *
   * @param V the value to query
   * @returns a set of declaration
   */
  set<llvm::Value *> declarations(llvm::Value &V);

  /**
   * Query the full def-use chain that the given value is involved in.
   *
   * @param V the value to query
   * @returns the set of declaration
   */
  set<llvm::Value *> chain(llvm::Value &V);

protected:
  set<llvm::Value *> decls;
  UnionFind<llvm::Value *> chains;

  friend class DefUseChainAnalysis;
};

} // namespace llvm::memoir

#endif // MEMOIR_ANALYSIS_DEFUSECHAIN_H
