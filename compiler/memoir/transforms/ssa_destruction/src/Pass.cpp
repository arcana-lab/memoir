#include <iostream>
#include <string>

// LLVM
#include "llvm/Pass.h"

#include "llvm/IR/CFG.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"

#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include "llvm/Analysis/DominanceFrontier.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

// NOELLE
#include "noelle/core/Noelle.hpp"

// MemOIR
#include "memoir/ir/Builder.hpp"
#include "memoir/ir/InstVisitor.hpp"
#include "memoir/ir/Instructions.hpp"

#include "memoir/analysis/TypeAnalysis.hpp"

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

static llvm::cl::opt<bool> DisableCollectionLowering(
    "disable-collection-lowering",
    llvm::cl::desc("Enable collection lowering"));

struct SSADestructionPass : public ModulePass {
  static char ID;

  SSADestructionPass() : ModulePass(ID) {}

  bool doInitialization(llvm::Module &M) override {
    return false;
  }

  using DomTreeNode = llvm::DomTreeNodeBase<llvm::BasicBlock>;
  using DomTreeTraversalListTy = list<llvm::BasicBlock *>;
  static DomTreeTraversalListTy dfs_preorder_traversal_helper(
      DomTreeNode *root) {
    MEMOIR_NULL_CHECK(root, "Root of dfs preorder traversal is NULL!");

    DomTreeTraversalListTy traversal = { root->getBlock() };

    for (auto *child : root->getChildren()) {
      auto child_traversal = dfs_preorder_traversal_helper(child);
      traversal.insert(traversal.end(),
                       child_traversal.begin(),
                       child_traversal.end());
    }

    return std::move(traversal);
  }

  static DomTreeTraversalListTy dfs_preorder_traversal(
      llvm::DominatorTree &DT) {
    auto *root_node = DT.getRootNode();
    MEMOIR_NULL_CHECK(root_node, "Root node couldn't be found, blame NOELLE");

    return std::move(dfs_preorder_traversal_helper(root_node));
  }

  static DomTreeTraversalListTy dfs_postorder_traversal_helper(
      DomTreeNode *root) {
    MEMOIR_NULL_CHECK(root, "Root of dfs postorder traversal is NULL!");

    DomTreeTraversalListTy traversal = {};

    for (auto *child : root->getChildren()) {
      auto child_traversal = dfs_preorder_traversal_helper(child);
      traversal.insert(traversal.end(),
                       child_traversal.begin(),
                       child_traversal.end());
    }

    traversal.push_back(root->getBlock());

    return std::move(traversal);
  }

  static DomTreeTraversalListTy dfs_postorder_traversal(
      llvm::DominatorTree &DT) {
    auto *root_node = DT.getRootNode();
    MEMOIR_NULL_CHECK(root_node, "Root node couldn't be found, blame NOELLE");

    return std::move(dfs_postorder_traversal_helper(root_node));
  }

  bool runOnModule(llvm::Module &M) override {
    infoln("BEGIN SSA Destruction pass");
    infoln();

    TypeAnalysis::invalidate();

    // Get NOELLE.
    auto &NOELLE = getAnalysis<arcana::noelle::Noelle>();

    SSADestructionStats stats;

    // Initialize the reaching definitions.
    SSADestructionVisitor SSADV(M, &stats, !DisableCollectionLowering);

    for (auto &F : M) {
      if (F.empty()) {
        continue;
      }

      bool no_memoir = true;
      for (auto &A : F.args()) {
        if (Type::value_is_collection_type(A)
            || Type::value_is_struct_type(A)) {
          TypeAnalysis::analyze(A);
          no_memoir = false;
        }
      }
      if (no_memoir) {
        for (auto &I : llvm::instructions(F)) {
          if (Type::value_is_collection_type(I) || Type::value_is_struct_type(I)
              || Type::value_is_type(I)) {
            TypeAnalysis::analyze(I);
            no_memoir = false;
          }
        }
      }
      if (no_memoir) {
        continue;
      }

      infoln();
      infoln("=========================");
      infoln("BEGIN: ", F.getName());

      // Get the dominator forest.
      auto &DT = getAnalysis<llvm::DominatorTreeWrapperPass>(F).getDomTree();

      // Compute the liveness analysis.
      auto LA = LivenessAnalysis(F, NOELLE.getDataFlowEngine());

      // Get a new value numbering instance.
      // auto VN = ValueNumbering(M);

      // Hand over this function's analyses.
      SSADV.setAnalyses(DT, LA);
      // SSADV.setAnalyses(DT, LA, VN);

      // Get the depth-first, preorder traversal of the dominator tree rooted at
      // the entry basic block.
      auto &entry_bb = F.getEntryBlock();
      auto dfs_preorder = dfs_preorder_traversal(DT);

      // Apply rewrite rules and renaming for reaching definitions.
      infoln("Coallescing collection variables");
      for (auto *bb : dfs_preorder) {
        for (auto &I : *bb) {
          SSADV.visit(I);
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

    TypeAnalysis::invalidate();

    return true;
  }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.addRequired<arcana::noelle::Noelle>();
    AU.addRequired<llvm::DominatorTreeWrapperPass>();
    AU.addRequired<llvm::DominanceFrontierWrapperPass>();
    return;
  }
};

} // namespace llvm::memoir

// Next there is code to register your pass to "opt"
char SSADestructionPass::ID = 0;
static RegisterPass<SSADestructionPass> X("ssa-destruction",
                                          "Destructs the MemOIR SSA form.");
