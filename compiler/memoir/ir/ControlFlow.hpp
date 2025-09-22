#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"

namespace memoir {

template <typename T>
T *parent(llvm::Value &value);

}
