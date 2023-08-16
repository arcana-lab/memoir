#include "memoir/utility/FunctionNames.hpp"
#include "memoir/support/Print.hpp"

namespace llvm::memoir {

std::ostream &operator<<(std::ostream &os, MemOIR_Func Enum) {
  switch (Enum) {
    default:
      os << "NONE";
      break;
#define HANDLE_INST(ENUM, FUNC, CLASS)                                         \
  case MemOIR_Func::ENUM:                                                      \
    os << #ENUM;                                                               \
    break;
  }
  return os;
}
llvm::raw_ostream &operator<<(llvm::raw_ostream &os, MemOIR_Func Enum) {
  switch (Enum) {
    default:
      os << "NONE";
      break;
#define HANDLE_INST(ENUM, FUNC, CLASS)                                         \
  case MemOIR_Func::ENUM:                                                      \
    os << #ENUM;                                                               \
    break;
  }
  return os;
}

bool FunctionNames::is_memoir_call(llvm::Function &function) {
  auto memoir_enum = FunctionNames::get_memoir_enum(function);

  return (memoir_enum != MemOIR_Func::NONE);
}

bool FunctionNames::is_memoir_call(llvm::CallInst &call_inst) {
  auto *callee = call_inst.getCalledFunction();
  if (callee == nullptr) {
    return false;
  }

  return is_memoir_call(*callee);
}

MemOIR_Func FunctionNames::get_memoir_enum(llvm::Function &function) {
  auto function_name = function.getName().str();

#define HANDLE_INST(MEMOIR_ENUM, MEMOIR_STR, _)                                \
  if (function_name == #MEMOIR_STR) {                                          \
    return MemOIR_Func::MEMOIR_ENUM;                                           \
  }
#include "memoir/ir/Instructions.def"
  return MemOIR_Func::NONE;
}

MemOIR_Func FunctionNames::get_memoir_enum(llvm::CallInst &call_inst) {
  auto *callee = call_inst.getCalledFunction();

  if (callee == nullptr) {
    return MemOIR_Func::NONE;
  }

  return get_memoir_enum(*callee);
}

Function *FunctionNames::get_memoir_function(Module &M,
                                             MemOIR_Func function_enum) {
  switch (function_enum) {
    default:
      return nullptr;
#define HANDLE_INST(MEMOIR_ENUM, MEMOIR_STR, _)                                \
  case MemOIR_Func::MEMOIR_ENUM:                                               \
    return M.getFunction(#MEMOIR_STR);
#include "memoir/ir/Instructions.def"
  }
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
    case MemOIR_Func::UINT64_TYPE:
    case MemOIR_Func::UINT32_TYPE:
    case MemOIR_Func::UINT16_TYPE:
    case MemOIR_Func::UINT8_TYPE:
    case MemOIR_Func::INT64_TYPE:
    case MemOIR_Func::INT32_TYPE:
    case MemOIR_Func::INT16_TYPE:
    case MemOIR_Func::INT8_TYPE:
    case MemOIR_Func::FLOAT_TYPE:
    case MemOIR_Func::DOUBLE_TYPE:
    case MemOIR_Func::POINTER_TYPE:
      return true;
    default:
      return false;
  }
}

// TODO: add HANDLE_STRUCT_TYPE_INST and HANDLE_COLLECTION_TYPE_INST to
// Instructions.def
bool FunctionNames::is_object_type(MemOIR_Func function_enum) {
  switch (function_enum) {
    case MemOIR_Func::DEFINE_STRUCT_TYPE:
    case MemOIR_Func::STRUCT_TYPE:
    case MemOIR_Func::TENSOR_TYPE:
    case MemOIR_Func::ASSOC_ARRAY_TYPE:
    case MemOIR_Func::SEQUENCE_TYPE:
      return true;
    default:
      return false;
  }
}

// TODO: add HANDLE_REFERENCE_TYPE_INST to Instructions.def
bool FunctionNames::is_reference_type(MemOIR_Func function_enum) {
  switch (function_enum) {
    case MemOIR_Func::REFERENCE_TYPE:
      return true;
    default:
      return false;
  }
}

bool FunctionNames::is_mutator(Module &M, MemOIR_Func function_enum) {
  auto *memoir_function = get_memoir_function(M, function_enum);
  if (!memoir_function) {
    return false;
  }
  return is_mutator(*memoir_function);
}

bool FunctionNames::is_mutator(Function &F) {
  return !(F.hasFnAttribute(llvm::Attribute::AttrKind::ReadNone));
}

// const map<MemOIR_Func, std::string> FunctionNames::memoir_to_function_names =
// { #define HANDLE_INST(MemOIR_Enum, MemOIR_Str, _) { MemOIR_Enum, #MemOIR_Str
// }, #include "memoir/ir/Instructions.def" #undef HANDLE_INST
// };

// const map<std::string, MemOIR_Func> FunctionNames::function_names_to_memoir =
// { #define HANDLE_INST(MemOIR_Enum, MemOIR_Str, _) { #MemOIR_Str, MemOIR_Enum
// }, #include "memoir/ir/Instructions.def" #undef HANDLE_INST
// };

}; // namespace llvm::memoir
