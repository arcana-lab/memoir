#include "common/utility/FunctionNames.hpp"

namespace llvm::memoir {

bool isMemOIRCall(llvm::Function &function) {
  auto function_name = function.getName();

  return FunctionNamesToMemOIR.find(function_name)
         != FunctionNamesToMemOIR.end();
}

bool isMemOIRCall(llvm::CallInst &call_inst) {
  auto callee = call_inst.getCalledFunction();
  if (callee == nullptr) {
    return false;
  }

  return isMemOIRCall(*callee);
}

MemOIR_Func getMemOIREnum(llvm::Function &function) {
  auto function_name = function.getName();

  auto found_iter = FunctionNamesToMemOIR.find(function_name);
  if (found_iter != FunctionNamesToMemOIR.end()) {
    auto found_enum = *found_iter;
    return found_enum.second;
  }
  return MemOIR_Func::NONE;
}

Function *getMemOIRFunction(Module &M, MemOIR_Func function_enum) {
  auto found_name = MemOIRToFunctionNames.find(function_enum);
  if (found_name == MemOIRToFunctionNames.end()) {
    return nullptr;
  }

  auto function_name = (*found_name).second;
  auto function = M.getFunction(function_name);

  return function;
}

bool isPrimitiveType(MemOIR_Func function_enum) {
  switch (function_enum) {
    case UINT64_TYPE:
    case UINT32_TYPE:
    case UINT16_TYPE:
    case UINT8_TYPE:
    case INT64_TYPE:
    case INT32_TYPE:
    case INT16_TYPE:
    case INT8_TYPE:
    case FLOAT_TYPE:
    case DOUBLE_TYPE:
      return true;
  }

  return false;
}

bool isObjectType(MemOIR_Func function_enum) {}

bool isReferenceType(MemOIR_Func function_enum) {}

}; // namespace llvm::memoir
