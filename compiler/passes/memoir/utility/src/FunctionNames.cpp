#include "common/utility/FunctionNames.hpp"

namespace llvm::memoir {

bool FunctionNames::is_memoir_call(llvm::Function &function) {
  auto function_name = function.getName();

  return FunctionNamesToMemOIR.find(function_name)
         != FunctionNamesToMemOIR.end();
}

bool FunctionNames::is_memoir_call(llvm::CallInst &call_inst) {
  auto callee = call_inst.getCalledFunction();
  if (callee == nullptr) {
    return false;
  }

  return isMemOIRCall(*callee);
}

MemOIR_Func FunctionNames::get_memoir_enum(llvm::Function &function) {
  auto function_name = function.getName();

  auto found_iter = FunctionNamesToMemOIR.find(function_name);
  if (found_iter != FunctionNamesToMemOIR.end()) {
    auto found_enum = *found_iter;
    return found_enum.second;
  }

  return MemOIR_Func::NONE;
}

MemOIR_Func FunctionNames::get_memoir_enum(llvm::CallInst &call_inst) {
  auto callee = call_inst.getCalledFunction();

  if (!callee) {
    return MemOIR_Func::NONE;
  }

  return getMemOIREnum(*callee);
}

Function *FunctionNames::get_memoir_function(Module &M,
                                             MemOIR_Func function_enum) {
  auto found_name = MemOIRToFunctionNames.find(function_enum);
  if (found_name == MemOIRToFunctionNames.end()) {
    return nullptr;
  }

  auto function_name = (*found_name).second;
  auto function = M.getFunction(function_name);

  return function;
}

bool FunctionNames::is_allocation(MemOIR_Func function_enum) {
  switch (function_enum) {
    case ALLOCATE_STRUCT:
    case ALLOCATE_TENSOR:
    case ALLOCATE_ASSOC_ARRAY:
    case ALLOCATE_SEQUENCE:
      return true;
    default:
      return false;
  }
}

bool FunctionNames::is_access(MemOIR_Func function_enum) {
  switch (function_enum) {
    default:
    case READ_UINT64:
    case READ_UINT32:
    case READ_UINT16:
    case READ_UINT8:
    case READ_INT64:
    case READ_INT32:
    case READ_INT16:
    case READ_INT8:
    case READ_BOOL:
    case WRITE_UINT64:
    case WRITE_UINT32:
    case WRITE_UINT16:
    case WRITE_UINT8:
    case WRITE_INT64:
    case WRITE_INT32:
    case WRITE_INT16:
    case WRITE_INT8:
    case WRITE_BOOL:
    case GET_OBJECT:
      return true;
    default:
      return false;
  }
}

bool FunctionNames::is_read(MemOIR_Func function_enum) {
  switch (function_enum) {
    default:
    case READ_UINT64:
    case READ_UINT32:
    case READ_UINT16:
    case READ_UINT8:
    case READ_INT64:
    case READ_INT32:
    case READ_INT16:
    case READ_INT8:
    case READ_BOOL:
      return true;
    default:
      return false;
  }
}

bool FunctionNames::is_write(MemOIR_Func function_enum) {
  switch (function_enum) {
    default:
    case WRITE_UINT64:
    case WRITE_UINT32:
    case WRITE_UINT16:
    case WRITE_UINT8:
    case WRITE_INT64:
    case WRITE_INT32:
    case WRITE_INT16:
    case WRITE_INT8:
    case WRITE_BOOL:
      return true;
    default:
      return false;
  }
}

bool FunctionNames::is_get(MemOIR_Func function_enum) {
  switch (function_enum) {
    case GET_OBJECT:
      return true;
    default:
      return false;
  }
}

bool FunctionNames::is_type(MemOIR_Func function_enum) {
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
    case POINTER_TYPE:
    case REFERENCE_TYPE:
    case STRUCT_TYPE:
    case TENSOR_TYPE:
    case STATIC_TENSOR_TYPE:
    case ASSOC_ARRAY_TYPE:
    case SEQUENCE_TYPE:
      return true;
    default:
      return false;
  }
}

bool FunctionNames::is_type(MemOIR_Func function_enum) {
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
    case POINTER_TYPE:
    case STRUCT_TYPE:
    case TENSOR_TYPE:
    case ASSOC_ARRAY_TYPE:
    case SEQUENCE_TYPE:
    case REFERENCE_TYPE:
      return true;
    default:
      return false;
  }
}

bool FunctionNames::is_primitive_type(MemOIR_Func function_enum) {
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
    case POINTER_TYPE:
      return true;
    default:
      return false;
  }
}

bool FunctionNames::is_object_type(MemOIR_Func function_enum) {
  switch (function_enum) {
    case STRUCT_TYPE:
    case TENSOR_TYPE:
    case ASSOC_ARRAY_TYPE:
    case SEQUENCE_TYPE:
      return true;
    default:
      return false;
  }
}

bool FunctionNames::is_reference_type(MemOIR_Func function_enum) {
  switch (function_enum) {
    case REFERENCE_TYPE:
      return true;
    default:
      return false;
  }
}

}; // namespace llvm::memoir
