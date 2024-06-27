#ifndef MEMOIR_PASSES_PASSES_H
#define MEMOIR_PASSES_PASSES_H

// LLVM
#include "llvm/Pass.h"

#include "llvm/IR/PassManager.h"

#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

#include "llvm/Support/CommandLine.h"

namespace llvm::memoir {

#define PASS(SCOPE, CLASS, NAME, ARGS...)                                      \
  struct CLASS : public llvm::PassInfoMixin<CLASS> {                           \
    llvm::PreservedAnalyses run(llvm::SCOPE &,                                 \
                                llvm::SCOPE##AnalysisManager &);               \
  };

#define ANALYSIS(SCOPE, CLASS, RESULT, ARGS...)                                \
  struct RESULT;                                                               \
  class PASS_NAME : public llvm::AnalysisInfoMixin<PASS_NAME> {                \
    friend struct llvm::AnalysisInfoMixin<PASS_NAME>;                          \
    static llvm::AnalysisKey Key;                                              \
                                                                               \
  public:                                                                      \
    using Result = RESULT;                                                     \
    Result run(llvm::SCOPE &M, llvm::SCOPE##AnalysisManager &MAM);             \
  };

#include "memoir/passes/Passes.def"
#undef PASS
#undef ANALYSIS

} // namespace llvm::memoir

// A helper macro to get a FunctionAnalysisManager from a ModuleAnalysisManager
#define GET_FUNCTION_ANALYSIS_MANAGER(_MAM, _MODULE)                           \
  _MAM.getResult<FunctionAnalysisManagerModuleProxy>(_MODULE).getManager()

// A helper macro to get a ModuleAnalysisManager from a FunctionAnalysisManager
#define GET_MODULE_ANALYSIS_MANAGER(_FAM, _FUNCTION)                           \
  _FAM.getResult<ModuleAnalysisManagerFunctionProxy>(_FUNCTION).getManager()

#endif // MEMOIR_PASSES_PASSES_H
