#include "FunctionNames.hpp"

namespace llvm::memoir {

bool isMemOIRCall(std::string function_name) {
  return FunctionNamesToMemOIR.find(functionName)
         != FunctionNamesToMemOIR.end();
}

MemOIRFunc getMemOIREnum(std::string function_name) {
  auto find_iter = FunctionNamesToMemOIR.find(functionName);
  if (find_iter != FunctionNamesToMemOIR.end()) {
    return find_iter.second;
  }
  return MemOIRFunc::NONE;
}

Function *getMemOIRFunction(Module &M, MemOIRFun function_enum) {
  auto function_name = getMemOIREnum(function_enum);
  auto function = M.getFunction(func_name);

  return function;
}

}; // namespace llvm::memoir
