#include <iostream>
#include <string>

// LLVM
#include "llvm/IR/CFG.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"

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
#include "memoir/support/DataTypes.hpp"
#include "memoir/support/Print.hpp"
#include "memoir/support/Timer.hpp"

#include "memoir/utility/FunctionNames.hpp"

#include "memoir/lower/SSADestruction.hpp"

using namespace memoir;

/*
 * This pass destructs the SSA representation, lowering it down to Collections
 * and Views.
 *
 * Author(s): Tommy McMichen
 * Created: August 7, 2023
 */

namespace memoir {

using DomTreeNode = llvm::DomTreeNodeBase<llvm::BasicBlock>;
using DomTreeTraversalListTy = List<llvm::BasicBlock *>;
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

static void cleanup_casts(llvm::Module &module) {
  // Collect all NOP cast operations.
  Set<llvm::Instruction *> to_delete = {};
  for (auto &func : module) {
    for (auto &inst : llvm::instructions(func)) {
      if (auto *cast = dyn_cast<llvm::CastInst>(&inst)) {

        if (cast->getSrcTy() == cast->getDestTy()) {

          // Short-circuit the cast.
          auto *operand = cast->getOperand(0);
          cast->replaceAllUsesWith(operand);

          // Mark the instruction to delete.
          to_delete.insert(cast);
        }
      }
    }
  }

  // Delete all instructions we found.
  for (auto *inst : to_delete) {
    inst->eraseFromParent();
  }
}

llvm::PreservedAnalyses SSADestructionPass::run(
    llvm::Module &M,
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
    for (auto *bb : dfs_preorder) {
      for (auto &I : *bb) {
        if (into<FoldInst>(I) or into<AllocInst>(I)) {
          SSADV.stage(I);
          continue;
        }
        SSADV.visit(I);
      }
    }

    // Recursively destruct instructions until none are staged.
    while (not SSADV.staged().empty()) {
      // Copy the current stage and clear the next stage.
      auto stage = SSADV.staged();
      SSADV.clear_stage();

      // Visit each of the staged instructions.
      for (auto *inst : stage) {
        SSADV.visit(*inst);
      }
    }

    // Finally, clean up any leftover type information.
    for (auto &BB : F) {
      for (auto &I : BB) {
        if (auto *type = into<TypeInst>(I)) {
          SSADV.visit(I);
        }
      }
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
  cleanup_casts(M);

  // Verify each function.
  for (auto &F : M) {
    if (not F.empty()) {
      if (llvm::verifyFunction(F, &llvm::errs())) {
        println(F);
        MEMOIR_UNREACHABLE("Failed to verify ", F.getName());
      }
    }
  }
  println("Verified module post-SSA destruction.");

  infoln("=========================");
  infoln("DONE SSA Destruction pass");
  infoln();

  return llvm::PreservedAnalyses::none();
}

} // namespace memoir
