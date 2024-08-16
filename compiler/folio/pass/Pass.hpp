#ifndef FOLIO_PASS_H
#define FOLIO_PASS_H

#include "llvm/Pass.h"

#include "llvm/IR/PassManager.h"

#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

#include "llvm/Support/CommandLine.h"

namespace folio {

// A helper macro to get a FunctionAnalysisManager from a ModuleAnalysisManager
#define GET_FUNCTION_ANALYSIS_MANAGER(_MAM, _MODULE)                           \
  _MAM.getResult<FunctionAnalysisManagerModuleProxy>(_MODULE).getManager()

// A helper macro to get a ModuleAnalysisManager from a FunctionAnalysisManager
#define GET_MODULE_ANALYSIS_MANAGER(_FAM, _FUNCTION)                           \
  _FAM.getResult<ModuleAnalysisManagerFunctionProxy>(_FUNCTION)

class FolioPass : public llvm::PassInfoMixin<FolioPass> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &MAM);
};

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
    Result run(llvm::SCOPE &M, llvm::SCOPE##AnalysisManager &MAM);             \
  };

#include "memoir/pass/Passes.def"
#undef PASS
#undef ANALYSIS

} // namespace folio

#endif // FOLIO_PASS_H
