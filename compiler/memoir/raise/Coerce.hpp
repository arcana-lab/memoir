#ifndef MEMOIR_RAISING_COERCE_H
#define MEMOIR_RAISING_COERCE_H

#include "llvm/IR/Use.h"
#include "llvm/IR/User.h"

namespace memoir {

/**
 * Coerces the operands of the user.
 * @return true if modified
 */
bool coerce(llvm::User &user);

} // namespace memoir

#endif
