#ifndef MEMOIR_TRANSFORMS_UTILITIES_PROMOTEGLOBALS_H
#define MEMOIR_TRANSFORMS_UTILITIES_PROMOTEGLOBALS_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/GlobalVariable.h"

namespace llvm::memoir {

/**
 * A quick check to see if the given global can be promoted, there are more
 * costly checks that need to happen. It is not guaranteed that all
 * applicability guards in {@link promote_global} will also pass.
 */
bool global_is_promotable(llvm::GlobalVariable &global);

/**
 * Promote the given global to a local variable.
 */
bool promote_global(llvm::GlobalVariable &global);

/**
 * Promote the given globals to local variables.
 */
bool promote_globals(llvm::ArrayRef<llvm::GlobalVariable *> globals);

} // namespace llvm::memoir

#endif // MEMOIR_TRANSFORMS_UTILITIES_PROMOTEGLOBALS_H
