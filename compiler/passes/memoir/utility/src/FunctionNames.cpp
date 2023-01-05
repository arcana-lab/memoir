#include "memoir/utility/FunctionNames.hpp"

namespace llvm::memoir {

bool FunctionNames::is_memoir_call(llvm::Function &function) {
  auto function_name = function.getName();

  return function_names_to_memoir.find(function_name)
         != function_names_to_memoir.end();
}

bool FunctionNames::is_memoir_call(llvm::CallInst &call_inst) {
  auto callee = call_inst.getCalledFunction();
  if (callee == nullptr) {
    return false;
  }

  return is_memoir_call(*callee);
}

MemOIR_Func FunctionNames::get_memoir_enum(llvm::Function &function) {
  auto function_name = function.getName();

  auto found_iter = function_names_to_memoir.find(function_name);
  if (found_iter != function_names_to_memoir.end()) {
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

  return get_memoir_enum(*callee);
}

Function *FunctionNames::get_memoir_function(Module &M,
                                             MemOIR_Func function_enum) {
  auto found_name = memoir_to_function_names.find(function_enum);
  if (found_name == memoir_to_function_names.end()) {
    return nullptr;
  }

  auto function_name = (*found_name).second;
  auto function = M.getFunction(function_name);

  return function;
}

bool FunctionNames::is_allocation(MemOIR_Func function_enum) {
  switch (function_enum) {
    default:
      return false;
#define HANDLE_ALLOC_INST(ENUM, FUNC, CLASS) case MemOIR_Func::ENUM:
#include "memoir/ir/Instructions.def"
      return true;
  }
}

bool FunctionNames::is_access(MemOIR_Func function_enum) {
  switch (function_enum) {
    default:
      return false;
#define HANDLE_ACCESS_INST(ENUM, FUNC, CLASS) case MemOIR_Func::ENUM:
#include "memoir/ir/Instructions.def"
      return true;
  }
}

bool FunctionNames::is_read(MemOIR_Func function_enum) {
  switch (function_enum) {
    default:
      return false;
#define HANDLE_READ_INST(ENUM, FUNC, CLASS) case MemOIR_Func::ENUM:
#include "memoir/ir/Instructions.def"
      return true;
  }
}

bool FunctionNames::is_write(MemOIR_Func function_enum) {
  switch (function_enum) {
    default:
      return false;
#define HANDLE_WRITE_INST(ENUM, FUNC, CLASS) case MemOIR_Func::ENUM:
#include "memoir/ir/Instructions.def"
      return true;
  }
}

bool FunctionNames::is_get(MemOIR_Func function_enum) {
  switch (function_enum) {
    default:
      return false;
#define HANDLE_GET_INST(ENUM, FUNC, CLASS) case MemOIR_Func::ENUM:
#include "memoir/ir/Instructions.def"
      return true;
  }
}

bool FunctionNames::is_type(MemOIR_Func function_enum) {
  switch (function_enum) {
    default:
      return false;
#define HANDLE_TYPE_INST(ENUM, FUNC, CLASS) case MemOIR_Func::ENUM:
#include "memoir/ir/Instructions.def"
      return true;
  }
}

// TODO: add HANDLE_PRIMITIVE_TYPE_INST to Instructions.def
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

// TODO: add HANDLE_STRUCT_TYPE_INST and HANDLE_COLLECTION_TYPE_INST to
// Instructions.def
bool FunctionNames::is_object_type(MemOIR_Func function_enum) {
  switch (function_enum) {
    case DEFINE_STRUCT_TYPE:
    case STRUCT_TYPE:
    case TENSOR_TYPE:
    case ASSOC_ARRAY_TYPE:
    case SEQUENCE_TYPE:
      return true;
    default:
      return false;
  }
}

// TODO: add HANDLE_REFERENCE_TYPE_INST to Instructions.def
bool FunctionNames::is_reference_type(MemOIR_Func function_enum) {
  switch (function_enum) {
    case REFERENCE_TYPE:
      return true;
    default:
      return false;
  }
}

const map<MemOIR_Func, std::string> FunctionNames::memoir_to_function_names = {
#define HANDLE_INST(MemOIR_Enum, MemOIR_Str, _) { MemOIR_Enum, #MemOIR_Str },
#include "memoir/ir/Instructions.def"
#undef HANDLE_INST
};

const map<std::string, MemOIR_Func> FunctionNames::function_names_to_memoir = {
#define HANDLE_INST(MemOIR_Enum, MemOIR_Str, _) { #MemOIR_Str, MemOIR_Enum },
#include "memoir/ir/Instructions.def"
#undef HANDLE_INST
};

}; // namespace llvm::memoir
