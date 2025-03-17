#ifndef MEMOIR_TRANSFORMS_UTILS_MUTATETYPES_H
#define MEMOIR_TRANSFORMS_UTILS_MUTATETYPES_H

#include "memoir/ir/Instructions.hpp"
#include "memoir/ir/Types.hpp"
#include "memoir/support/InternalDatatypes.hpp"

namespace llvm::memoir {

#if 0

/**
 * Mutates the type of the given allocation.
 * @param alloc the allocation to mutate.
 * @param type the new type.
 * @returns a vmap of all new values.
 */
map<llvm::Value *, llvm::Value *> mutate_type(AllocInst &alloc, Type &type);

#endif

} // namespace llvm::memoir

#endif // MEMOIR_TRANSFORMS_UTILS_MUTATETYPES_H
