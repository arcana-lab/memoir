#include "FunctionNames.hpp"

namespace llvm::memoir {

bool isMemOIRCall(std::string function_name) {
  return FunctionNamesToMemOIR.find(functionName)
         != FunctionNamesToMemOIR.end();
}

MemOIRFunc getMemOIRCall(std::string function_name) {
  auto find_iter = FunctionNamesToMemOIR.find(functionName);
  if (find_iter != FunctionNamesToMemOIR.end()) {
    return find_iter.second;
  }
  return MemOIRFunc::NONE;
}

}; // namespace llvm::memoir
