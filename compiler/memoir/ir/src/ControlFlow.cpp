#include "memoir/ir/ControlFlow.hpp"

namespace llvm::memoir {

llvm::BasicBlock *parent<llvm::BasicBlock>(llvm::value &value) {
  if (auto *inst = dyn_cast<llvm::Instruction>(&value))
    return inst->getParent();
  return NULL;
}

llvm::Function *parent<llvm::Function>(llvm::Value &value) {
  if (auto *inst = dyn_cast<llvm::Instruction>(&value))
    return inst->getFunction();
  else if (auto *arg = dyn_cast<llvm::Argument>(&value))
    return arg->getParent();
  return NULL;
}

llvm::Module *parent<llvm::Module>(llvm::Value &value) {
  if (auto *func = parent<llvm::Function>(value))
    return func->getParent();
  if (auto *global = dyn_cast<llvm::GlobalValue>(&value))
    return global->getParent();
  return NULL;
}

} // namespace llvm::memoir
