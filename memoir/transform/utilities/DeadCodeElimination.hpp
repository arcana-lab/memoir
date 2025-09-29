#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/IR/Function.h"

namespace memoir {

/**
 * Eliminate dead code in the given function.
 */
bool eliminate_dead_code(llvm::Function &function,
                         llvm::TargetLibraryInfo *TLI);

} // namespace memoir
