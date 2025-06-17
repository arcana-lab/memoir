#ifndef MEMOIR_IR_CALLGRAPH_H
#define MEMOIR_IR_CALLGRAPH_H

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"

#include "memoir/support/DataTypes.hpp"

namespace llvm::memoir {

/**
 * Special value for the unknown caller.
 */
llvm::CallBase *unknown_caller();

/**
 * Check if this caller is the unknown caller.
 */
bool is_unknown_caller(const llvm::CallBase *caller);

/**
 * Collect the set of possible callers to the given function.
 */
Set<llvm::CallBase *> possible_callers(llvm::Function &function);

} // namespace llvm::memoir

#endif // MEMOIR_IR_CALLGRAPH_H
