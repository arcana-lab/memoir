#include "memoir/ir/Function.hpp"

namespace llvm::memoir {

MemOIRFunctionType::MemOIRFunctionType(llvm::FunctionType &FT,
                                       Type *return_type,
                                       vector<Type *> param_types)
  : FT(FT),
    return_type(return_type),
    param_types(param_types) {
  // Do nothing.
}

llvm::FunctionType &MemOIRFunctionType::getLLVMFunctionType() const {
  return this->FT;
}

Type *MemOIRFunctionType::getReturnType() const {
  return this->return_type;
}

llvm::Type *MemOIRFunctionType::getReturnLLVMType() const {
  return this->getLLVMFunctionType().getReturnType();
}

unsigned MemOIRFunctionType::getNumParams() const {
  return this->getLLVMFunctionType().getNumParams();
}

Type *MemOIRFunctionType::getParamType(unsigned param_index) const {
  MEMOIR_ASSERT((param_index < this->param_types.size()),
                "Attempt to get out-of-range parameter type");

  return this->param_types.get(param_index);
}

llvm::Type *MemOIRFunctionType::getParamLLVMType(unsigned param_index) const {
  return this->getLLVMFunctionType().getParamType(param_index);
}

MemOIRFunctionType::~MemOIRFunctionType() {
  // Do nothing.
}

} // namespace llvm::memoir
