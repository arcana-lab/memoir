#include "llvm/IR/Dominators.h"

#include "memoir/support/InternalDatatypes.hpp"

namespace llvm::memoir {

List<llvm::BasicBlock *> dfs_preorder_traversal(llvm::DominatorTree &DT);

}
