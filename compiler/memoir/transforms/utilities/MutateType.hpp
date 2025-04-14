#ifndef MEMOIR_TRANSFORMS_UTILS_MUTATETYPES_H
#define MEMOIR_TRANSFORMS_UTILS_MUTATETYPES_H

#include "llvm/IR/Function.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include "memoir/ir/Instructions.hpp"
#include "memoir/ir/Types.hpp"
#include "memoir/support/DataTypes.hpp"

namespace llvm::memoir {

using OnFuncClone = std::function<void(llvm::Function & /* old function */,
                                       llvm::Function & /* new function */,
                                       llvm::ValueToValueMapTy & /* vmap */
                                       )>;

/**
 * The default callback for function cloning.
 */
void default_on_func_clone(llvm::Function &old_function,
                           llvm::Function &new_function,
                           llvm::ValueToValueMapTy &vmap) {
  // Do nothing.
  return;
}

/**
 * Mutates the type of the given allocation.
 * @param alloc the allocation to mutate.
 * @param type the new type.
 */
void mutate_type(AllocInst &alloc,
                 Type &type,
                 OnFuncClone on_func_clone = default_on_func_clone);

} // namespace llvm::memoir

#endif // MEMOIR_TRANSFORMS_UTILS_MUTATETYPES_H
