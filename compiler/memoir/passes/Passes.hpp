#ifndef MEMOIR_PASSES_PASSES_H
#define MEMOIR_PASSES_PASSES_H

// LLVM
#include "llvm/Pass.h"

#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"

#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

#include "llvm/Support/CommandLine.h"

// MEMOIR
#include "memoir/support/PassUtils.hpp"

namespace llvm::memoir {

#define PASS(SCOPE, CLASS, NAME, ARGS...)                                      \
  struct CLASS : public llvm::PassInfoMixin<CLASS> {                           \
    llvm::PreservedAnalyses run(llvm::SCOPE &,                                 \
                                llvm::SCOPE##AnalysisManager &);               \
  };

#define ANALYSIS(SCOPE, CLASS, RESULT, ARGS...)                                \
  struct RESULT;                                                               \
  class CLASS : public llvm::AnalysisInfoMixin<CLASS> {                        \
    friend struct llvm::AnalysisInfoMixin<CLASS>;                              \
    static llvm::AnalysisKey Key;                                              \
                                                                               \
  public:                                                                      \
    using Result = RESULT;                                                     \
    Result run(llvm::SCOPE &, llvm::SCOPE##AnalysisManager &);                 \
  };

#include "memoir/passes/Passes.def"
#undef PASS
#undef ANALYSIS

} // namespace llvm::memoir

#endif // MEMOIR_PASSES_PASSES_H
