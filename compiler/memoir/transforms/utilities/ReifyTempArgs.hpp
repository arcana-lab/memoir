#ifndef MEMOIR_TRANSFORMS_UTILITIES_REIFYTEMPARGS_H
#define MEMOIR_TRANSFORMS_UTILITIES_REIFYTEMPARGS_H

// LLVM
#include "llvm/IR/Module.h"

namespace memoir {

/**
 * Convert tempargs in the given LLVM module into formal arguments to the
 * function.
 *
 * @returns TRUE if the module was modified.
 */
bool reify_tempargs(llvm::Module &M);

} // namespace memoir

#endif // MEMOIR_TRANSFORMS_UTILITIES_REIFYTEMPARGS_H
