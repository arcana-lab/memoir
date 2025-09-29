#ifndef MEMOIR_TRANSFORMS_UTILITIES_INLINER_H
#define MEMOIR_TRANSFORMS_UTILITIES_INLINER_H

#include "llvm/Transforms/Utils/Cloning.h"

namespace memoir {

/**
 * A wrapper for LLVM's InlineFunction call.
 * Re-maps arguments to their live-out variable.
 */
llvm::InlineResult InlineFunction(llvm::CallBase &CB,
                                  llvm::InlineFunctionInfo &IFI,
                                  bool MergeAttributes = false,
                                  llvm::AAResults *CalleeAAR = nullptr,
                                  bool InsertLifetime = true,
                                  llvm::Function *ForwardArgsTo = nullptr);

} // namespace memoir

#endif // MEMOIR_TRANSFORMS_UTILITIES_INLINER_H
