#ifndef MEMOIR_RAISING_DEFUNCTIONALIZE_H
#define MEMOIR_RAISING_DEFUNCTIONALIZE_H

namespace memoir {

/**
 * Defunctionalize the given call
 * @param call the call to transform
 * @param possible_unknown the callee of this function may not be in the module
 * @returns true if the program was transformed, false otherwise
 */
bool defunctionalize(llvm::CallBase &call, bool possibly_unknown = true);

/**
 * Defunctionalize all indirect calls in the given module.
 * @param M the module to transform
 * @returns true if the module was transformed, false otherwise
 */
bool defunctionalize(llvm::Function &F);

/**
 * Defunctionalize all indirect calls in the given function.
 * @param F the function to transform
 * @returns true if the function was transformed, false otherwise
 */
bool defunctionalize(llvm::Function &F);

} // namespace memoir

#endif // MEMOIR_RAISING_DEFUNCTIONALIZE_H
