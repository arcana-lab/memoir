#ifndef FOLIO_LAMBDALIFTING_H
#define FOLIO_LAMBDALIFTING_H

#include "llvm/IR/Module.h"

namespace llvm::memoir {

/**
 * Perform lambda lifting on the given module, where each call is converted to
 * have a unique callee.
 * @param M the module to transform
 * @return true if transformed, false otherwise
 */
bool lambda_lift(llvm::Module &M);

} // namespace llvm::memoir

#endif // FOLIO_LAMBDALIFTING_H
