#include "memoir/ir/FunctionType.hpp"

namespace llvm::memoir {

llvm::FunctionType &FunctionType::getLLVMFunctionType() const {
  return this->FT;
}

std::variant<Type *, llvm::Type *> FunctionType::getReturnType() const {

  // If the MEMOIR return type is non-NULL, return it.
  if (this->return_type != nullptr) {
    return this->return_type;
  }

  // Otherwise, get the LLVM return type in the LLVM function type.
  return this->getLLVMFunctionType().getReturnType();
}

unsigned FunctionType::getNumParams() const {
  return this->getLLVMFunctionType().getNumParams();
}

std::variant<Type *, llvm::Type *> FunctionType::getParamType(
    unsigned param_index) const {
  MEMOIR_ASSERT((param_index < this->param_types.size()),
                "Attempt to get out-of-range parameter type");

  // If there's a MEMOIR type for this parameter, return it.
  auto found_param = this->param_types.find(param_index);
  if (found_param != this->param_types.end()) {
    return found_param->second;
  }

  // Otherwise, lookup the LLVM type in the LLVM function type.
  return this->getLLVMFunctionType().getParamType(param_index);
}

} // namespace llvm::memoir
