#ifndef MEMOIR_LOWERING_LOWERFOLD_H
#define MEMOIR_LOWERING_LOWERFOLD_H

// MEMOIR
#include "memoir/ir/Instructions.hpp"

namespace llvm::memoir {

bool lower_fold(
    FoldInst &I,
    llvm::Value &collection,
    Type &collection_type,
    llvm::Function *begin_func = nullptr,
    llvm::Function *next_func = nullptr,
    llvm::Type *iter_type = nullptr,
    std::function<void(llvm::Value &, llvm::Value &)> coalesce =
        [](llvm::Value &orig, llvm::Value &replacement) {
          orig.replaceAllUsesWith(&replacement);
        },
    std::function<void(llvm::Instruction &)> cleanup =
        [](llvm::Instruction &I) { I.eraseFromParent(); });

} // namespace llvm::memoir

#endif // MEMOIR_LOWERING_LOWERFOLD_H
