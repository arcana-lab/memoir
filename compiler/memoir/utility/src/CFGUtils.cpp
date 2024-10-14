#include "memoir/utility/CFGUtils.hpp"
#include "memoir/support/Assert.hpp"

namespace llvm::memoir {

namespace detail {

using DomTreeNode = llvm::DomTreeNodeBase<llvm::BasicBlock>;

void _dfs_preorder_traversal(DomTreeNode *root,
                             list<llvm::BasicBlock *> &traversal) {
  MEMOIR_NULL_CHECK(root, "Root of dfs preorder traversal is NULL");

  traversal.push_back(root->getBlock());

  for (auto *child : root->children()) {
    _dfs_preorder_traversal(child, traversal);
  }

  return;
}

} // namespace detail

list<llvm::BasicBlock *> dfs_preorder_traversal(llvm::DominatorTree &DT) {

  list<llvm::BasicBlock *> traversal = {};

  auto *root_node = DT.getRootNode();
  MEMOIR_NULL_CHECK(root_node, "Root node couldn't be found, blame LLVM");

  detail::_dfs_preorder_traversal(root_node, traversal);

  return traversal;
}

} // namespace llvm::memoir
