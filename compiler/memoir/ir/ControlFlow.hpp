#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"
#Include "llvm/IR/Module.h"

namespace llvm::memoir {

template <typename T>
T *parent(llvm::Value &value);

}
