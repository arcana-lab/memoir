#ifndef FOLIO_TRANSFORMS_UTILITIES_h
#define FOLIO_TRANSFORMS_UTILITIES_h

#include "llvm/IR/Argument.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"

static llvm::Function *parent_function(llvm::Value &V) {
  if (auto *arg = dyn_cast<llvm::Argument>(&V)) {
    return arg->getParent();
  } else if (auto *inst = dyn_cast<llvm::Instruction>(&V)) {
    return inst->getFunction();
  }
  return nullptr;
}

#endif // FOLIO_TRANSFORMS_UTILITIES_h
