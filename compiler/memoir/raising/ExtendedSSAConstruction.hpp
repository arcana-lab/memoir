#ifndef MEMOIR_RAISING_EXTENDEDSSACONSTRUCTION_H
#define MEMOIR_RAISING_EXTENDEDSSACONSTRUCTION_H

#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"

namespace memoir {

/**
 * Transform the input program to be in extended SSA form.
 *
 * @param F the function to transform
 * @param DT the dominator tree of F
 * @returns true if the function was transformed, false if no changes were made
 */
bool construct_extended_ssa(llvm::Function &F, llvm::DominatorTree &DT);

} // namespace memoir

#endif
