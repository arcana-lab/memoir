#include <iostream>
#include <string>

// LLVM
#include "llvm/IR/CFG.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"

#include "llvm/Analysis/DominanceFrontier.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

// MemOIR
#include "memoir/passes/Passes.hpp"

#include "memoir/ir/Builder.hpp"
#include "memoir/ir/InstVisitor.hpp"
#include "memoir/ir/Instructions.hpp"
#include "memoir/ir/Verifier.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/support/InternalDatatypes.hpp"
#include "memoir/support/Print.hpp"
#include "memoir/support/Timer.hpp"

#include "memoir/utility/FunctionNames.hpp"

#include "SSADestruction.hpp"

using namespace llvm::memoir;

/*
 * This pass destructs the SSA representation, lowering it down to Collections
 * and Views.
 *
 * Author(s): Tommy McMichen
 * Created: August 7, 2023
 */

namespace llvm::memoir {

using DomTreeNode = llvm::DomTreeNodeBase<llvm::BasicBlock>;
using DomTreeTraversalListTy = list<llvm::BasicBlock *>;
static DomTreeTraversalListTy dfs_preorder_traversal_helper(DomTreeNode *root) {
  MEMOIR_NULL_CHECK(root, "Root of dfs preorder traversal is NULL!");

  DomTreeTraversalListTy traversal = { root->getBlock() };

  for (auto *child : root->children()) {
    auto child_traversal = dfs_preorder_traversal_helper(child);
    traversal.insert(traversal.end(),
                     child_traversal.begin(),
                     child_traversal.end());
  }

  return traversal;
}

static DomTreeTraversalListTy dfs_preorder_traversal(llvm::DominatorTree &DT) {
  auto *root_node = DT.getRootNode();
  MEMOIR_NULL_CHECK(root_node, "Root node couldn't be found in DominatorTree.");

  return dfs_preorder_traversal_helper(root_node);
}

static DomTreeTraversalListTy dfs_postorder_traversal_helper(
    DomTreeNode *root) {
  MEMOIR_NULL_CHECK(root, "Root of dfs postorder traversal is NULL!");

  DomTreeTraversalListTy traversal = {};

  for (auto *child : root->children()) {
    auto child_traversal = dfs_preorder_traversal_helper(child);
    traversal.insert(traversal.end(),
                     child_traversal.begin(),
                     child_traversal.end());
  }

  traversal.push_back(root->getBlock());

  return traversal;
}

[[maybe_unused]] static DomTreeTraversalListTy dfs_postorder_traversal(
    llvm::DominatorTree &DT) {
  auto *root_node = DT.getRootNode();
  MEMOIR_NULL_CHECK(root_node, "Root node couldn't be found in DominatorTree");

  return dfs_postorder_traversal_helper(root_node);
}

PreservedAnalyses SSADestructionPass::run(llvm::Module &M,
                                          llvm::ModuleAnalysisManager &MAM) {

  // Verify the module.
  if (Verifier::verify(M, MAM)) {
    MEMOIR_UNREACHABLE("MEMOIR Verifier failed for the Module!");
  }

  infoln("BEGIN SSA Destruction pass");
  infoln();

  SSADestructionStats stats;

  // Initialize the reaching definitions.
  SSADestructionVisitor SSADV(M, &stats);

  // Get the function analysis manager.
  auto &FAM = GET_FUNCTION_ANALYSIS_MANAGER(MAM, M);

  for (auto &F : M) {
    if (F.empty()) {
      continue;
    }

    infoln();
    infoln("=========================");
    infoln("BEGIN: ", F.getName());

    // Get the dominator forest.
    auto &DT = FAM.getResult<llvm::DominatorTreeAnalysis>(F);

    // Get a new value numbering instance.
    // auto VN = ValueNumbering(M);

    // Hand over this function's analyses.
    SSADV.setAnalyses(DT);

    // Get the depth-first, preorder traversal of the dominator tree rooted at
    // the entry basic block.
    auto dfs_preorder = dfs_preorder_traversal(DT);

    // Apply rewrite rules and renaming for reaching definitions.
    infoln("Coallescing collection variables");
    set<llvm::Instruction *> folds = {};
    for (auto *bb : dfs_preorder) {
      for (auto &I : *bb) {
        if (into<FoldInst>(I)) {
          folds.insert(&I);
          continue;
        }
        SSADV.visit(I);
      }
    }

    for (auto *inst : folds) {
      SSADV.visit(*inst);
    }

    infoln("END: ", F.getName());
    infoln("=========================");
  }

  // Get the depth-first, preorder traversal of the dominator tree rooted at
  // the entry basic block.
  // auto dfs_postorder = dfs_postorder_traversal(DT);
  infoln("Performing the coalescence");
  for (auto &F : M) {
    if (F.empty()) {
      continue;
    }

    for (auto &BB : F) {
      // Reverse iterate on instructions in the basic block.
      for (auto it = BB.begin(); it != BB.end(); ++it) {
        auto &I = *it;
        SSADV.do_coalesce(I);
      }
    }
  }

  infoln("Cleaning up dead instructions.");
  SSADV.cleanup();

  infoln("=========================");
  infoln("DONE SSA Destruction pass");
  infoln();

  return llvm::PreservedAnalyses::none();
}

} // namespace llvm::memoir
