#include "common/ir/Function.hpp"

#include "common/support/Assert.hpp"

namespace llvm::memoir {

MemOIRFunction &MemOIRFunction::get(llvm::Function &F) {
  MemOIRModule::get(F.getParent()).getFunction(F);
}

MemOIRFunction::MemOIRFunction(llvm::Function &F) : F(F) {
  auto llvm_function_type = F.getFunctionType();
  MEMOIR_NULL_CHECK(
      llvm_function_type,
      "Attempt to construct a MemOIRFunction with NULL FunctionType");
  this->function_type = new MemOIRFunctionType(F.getFunctionType());
}

MemOIRModule &MemOIRFunction::getParent() const {
  return MemOIRModule::get(this->F.getParent());
}

MemOIRFunctionType &MemOIRFunction::getFunctionType() const {
  return *(this->function_type);
}

llvm::Function &MemOIRFunction::getLLVMFunction() const {
  return this->F;
}

unsigned MemOIRFunction::getNumberOfArguments() const {
  return this->getFunctionType().getNumParams();
}

Type *MemOIRFunction::getArgumentType(unsigned arg_index) const {
  return this->getFunctionType().getParamType(arg_index);
}

llvm::Type *MemOIRFunction::getArgumentLLVMType(unsigned arg_index) const {
  return this->getFunctionType().getParamLLVMType(arg_index);
}

llvm::Argument &MemOIRFunction::getArgument(unsigned arg_index) const {
  return this->getLLVMFunction().getArg(arg_index);
}

Type *MemOIRFunction::getReturnType() const {
  return this->getFunctionType().getReturnType();
}

llvm::Type *MemOIRFunction::getReturnLLVMType() const {
  return this->getFunctionType().getReturnLLVMType();
}

} // namespace llvm::memoir
