#ifndef MEMOIR_PASSES_PASSES_H
#define MEMOIR_PASSES_PASSES_H

// LLVM
#include "llvm/Pass.h"

#include "llvm/IR/PassManager.h"

#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

// Macro to declare passes.
#define MODULE_PASS(PASS_NAME)                                                 \
  struct PASS_NAME : public llvm::PassInfoMixin<PASS_NAME> {                   \
    llvm::PreservedAnalyses run(llvm::Module &M,                               \
                                llvm::ModuleAnalysisManager &MAM);             \
  }

#define ANALYSIS_PASS(SCOPE, PASS_NAME, RESULT)                                \
  struct RESULT;                                                               \
  class PASS_NAME : public llvm::AnalysisInfoMixin<PASS_NAME> {                \
    friend struct llvm::AnalysisInfoMixin<PASS_NAME>;                          \
    static llvm::AnalysisKey Key;                                              \
                                                                               \
  public:                                                                      \
    using Result = RESULT;                                                     \
    Result run(llvm::SCOPE &M, llvm::SCOPE##AnalysisManager &MAM);             \
  }

namespace llvm::memoir {

MODULE_PASS(SSAConstructionPass);

MODULE_PASS(SSADestructionPass);

MODULE_PASS(ImplLinkerPass);

MODULE_PASS(NormalizationPass);

MODULE_PASS(StatisticsPass);

MODULE_PASS(TypeInferencePass);

} // namespace llvm::memoir

// A helper macro to get a FunctionAnalysisManager from a ModuleAnalysisManager
#define GET_FUNCTION_ANALYSIS_MANAGER(_MAM, _MODULE)                           \
  _MAM.getResult<FunctionAnalysisManagerModuleProxy>(_MODULE).getManager()

#endif // MEMOIR_PASSES_PASSES_H
