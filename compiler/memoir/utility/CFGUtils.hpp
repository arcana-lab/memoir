#ifndef MEMOIR_SUPPORT_CFGUTILS_H
#define MEMOIR_SUPPORT_CFGUTILS_H

#include "llvm/IR/CFG.h"
#include "llvm/IR/Dominators.h"

#include "memoir/support/InternalDatatypes.hpp"

namespace llvm::memoir {

list<llvm::BasicBlock *> dfs_preorder_traversal(llvm::DominatorTree &DT);

}

#endif
