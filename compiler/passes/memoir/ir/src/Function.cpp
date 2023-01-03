#include "memoir/ir/Function.hpp"

#include "memoir/support/Assert.hpp"

namespace llvm::memoir {

MemOIRFunction &MemOIRFunction::get(llvm::Function &F) {
  MemOIRModule::get(F.getParent()).getFunction(F);
}

Type *MemOIRFunction::get_argument_type(llvm::Argument &A) {
  auto *llvm_func = A.getParent();
  MEMOIR_NULL_CHECK(llvm_func,
                    "Attempt to get argument type of unlinked argument");

  auto &memoir_func = MemOIRFunction::get(*llvm_func);
  return memoir_func.getArgumentType(A.getArgNo());
}

MemOIRFunction::MemOIRFunction(llvm::Function &F) : F(F) {
  auto llvm_function_type = F.getFunctionType();
  MEMOIR_NULL_CHECK(
      llvm_function_type,
      "Attempt to construct a MemOIRFunction with NULL FunctionType");
  this->function_type = new MemOIRFunctionType(F.getFunctionType());
}

llvm::Module &MemOIRFunction::getParent() const {
  return this->getLLVMFunction()->getParent();
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
  return *(this->getLLVMFunction().arg_begin() + arg_index);
}

Type *MemOIRFunction::getReturnType() const {
  return this->getFunctionType().getReturnType();
}

llvm::Type *MemOIRFunction::getReturnLLVMType() const {
  return this->getFunctionType().getReturnLLVMType();
}

MemOIRFunction::~MemOIRFunction() {
  delete function_type;
  for (auto memoir_inst : this->memoir_instructions) {
    delete memoir_inst;
  }
  this->memoir_instructions.clear();
}

map<llvm::Function *, MemOIRFunction *>
    MemOIRFunction::llvm_to_memoir_functions = {};

} // namespace llvm::memoir
