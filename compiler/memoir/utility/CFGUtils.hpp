#include "llvm/IR/Dominators.h"

#include "memoir/support/InternalDatatypes.hpp"

namespace llvm::memoir {

list<llvm::BasicBlock *> dfs_preorder_traversal(llvm::DominatorTree &DT);

}
