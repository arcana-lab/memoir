#ifndef MEMOIR_RAISING_REPAIRSSA_H
#define MEMOIR_RAISING_REPAIRSSA_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"

namespace memoir {

/**
 * Repair SSA form for the list of stack variables.
 * @param vars the stack variables to repair, they must all share a single
 * parent function
 * @param domtree the dominator tree for the parent function of all stack
 * variables
 */
void repair_ssa(llvm::ArrayRef<llvm::AllocaInst *> vars,
                llvm::DominatorTree &domtree);

/**
 * Repair SSA form for the list of stack variables.
 * @param vars the stack variables to repair
 * @param get_domtree a function to fetch the dominator tree of a function
 */
void repair_ssa(
    llvm::ArrayRef<llvm::AllocaInst *> vars,
    std::function<llvm::DominatorTree &(llvm::Function &)> get_domtree);

/**
 * Repairs SSA form for all object variables in the function.
 * @param F the function to repair
 */
void repair_ssa(llvm::Function &F, llvm::DominatorTree &DT);

} // namespace memoir

#endif // MEMOIR_RAISING_REPAIRSSA_H
