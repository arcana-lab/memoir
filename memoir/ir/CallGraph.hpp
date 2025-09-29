#ifndef MEMOIR_IR_CALLGRAPH_H
#define MEMOIR_IR_CALLGRAPH_H

#include "llvm/Analysis/CallGraph.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"

#include "memoir/support/DataTypes.hpp"

namespace memoir {

/**
 * An adaptor for the LLVM CallGraph, with additional information about MEMOIR
 * operations.
 */
struct CallGraph : public llvm::CallGraph {
  using Base = llvm::CallGraph;

  CallGraph(llvm::Module &M);
};

/**
 * Special value for the unknown caller.
 */
llvm::CallBase *unknown_caller();

/**
 * Check if this caller is the unknown caller.
 */
bool is_unknown_caller(const llvm::CallBase *caller);

/**
 *  Check if the given caller set has the unknown caller.
 */
bool has_unknown_caller(const Set<llvm::CallBase *> &callers);

/**
 * Collect the set of possible callers to the given function into the given set.
 * returns TRUE iff the set includes the unknown caller.
 */
bool possible_callers(llvm::Function &function, Set<llvm::CallBase *> &callers);

/**
 * Collect the set of possible callers to the given function.
 */
Set<llvm::CallBase *> possible_callers(llvm::Function &function);

/**
 * Fetch the single caller of the given function, if it exists.
 */
llvm::CallBase *single_caller(llvm::Function &function);

} // namespace memoir

#endif // MEMOIR_IR_CALLGRAPH_H
