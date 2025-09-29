#ifndef MEMOIR_SUPPORT_PASSUTILS_H
#define MEMOIR_SUPPORT_PASSUTILS_H

#include "llvm/IR/PassManager.h"

// A helper macro to get a FunctionAnalysisManager from a ModuleAnalysisManager
#define GET_FUNCTION_ANALYSIS_MANAGER(_MAM, _MODULE)                           \
  _MAM.getResult<llvm::FunctionAnalysisManagerModuleProxy>(_MODULE).getManager()

// A helper macro to get a ModuleAnalysisManager from a FunctionAnalysisManager
#define GET_MODULE_ANALYSIS_MANAGER(_FAM, _FUNCTION)                           \
  _FAM.getResult<llvm::ModuleAnalysisManagerFunctionProxy>(_FUNCTION)

#endif // MEMOIR_SUPPORT_PASSUTILS_H
