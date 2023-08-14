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
#include "noelle/core/DominatorSummary.hpp"
#include "noelle/core/Noelle.hpp"

// MemOIR
#include "memoir/ir/Builder.hpp"
#include "memoir/ir/Function.hpp"
#include "memoir/ir/InstVisitor.hpp"
#include "memoir/ir/Instructions.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/support/InternalDatatypes.hpp"
#include "memoir/support/Print.hpp"

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

struct SSADestructionPass : public ModulePass {
  static char ID;

  SSADestructionPass() : ModulePass(ID) {}

  bool doInitialization(Module &M) override {
    return false;
  }

  using DomTreeTraversalListTy = list<llvm::BasicBlock *>;
  static DomTreeTraversalListTy dfs_preorder_traversal_helper(
      llvm::noelle::DomNodeSummary *root) {
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
      llvm::noelle::DomTreeSummary &DT,
      llvm::BasicBlock &root) {
    auto *root_node = DT.getNode(&root);
    MEMOIR_NULL_CHECK(root_node, "Root node couldn't be found, blame NOELLE");

    return std::move(dfs_preorder_traversal_helper(root_node));
  }

  bool runOnModule(Module &M) override {
    println("BEGIN mut2immut pass");
    println();

    // Get NOELLE.
    auto &NOELLE = getAnalysis<llvm::noelle::Noelle>();

    SSADestructionStats stats;

    for (auto &F : M) {
      if (F.empty()) {
        continue;
      }

      println();
      println("=========================");
      println("BEGIN: ", F.getName());

      // Get the dominator forest.
      auto &DT = NOELLE.getDominators(&F)->DT;

      // Get the dominance frontier.
      auto &DF = getAnalysis<llvm::DominanceFrontierWrapperPass>(F)
                     .getDominanceFrontier();

      // Get the depth-first, preorder traversal of the dominator tree rooted at
      // the entry basic block.
      auto &entry_bb = F.getEntryBlock();
      auto dfs_traversal = dfs_preorder_traversal(DT, entry_bb);

      // Initialize the reaching definitions.
      SSADestructionVisitor SSADV(DT, &stats);

      // Apply rewrite rules and renaming for reaching definitions.
      println("Applying rewrite rules");
      for (auto *bb : dfs_traversal) {
        for (auto &I : *bb) {
          SSADV.visit(I);
        }
      }

      println("Cleaning up dead mutable instructions.");
      SSADV.cleanup();

      println("END: ", F.getName());
      println("=========================");
    }

    println("=========================");
    println("STATS SSA Destruction pass");
    println("=========================");
    println("DONE SSA Destruction pass");
    return true;
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<llvm::noelle::Noelle>();
    AU.addRequired<llvm::DominanceFrontierWrapperPass>();
    return;
  }
};

} // namespace llvm::memoir

// Next there is code to register your pass to "opt"
char SSADestructionPass::ID = 0;
static RegisterPass<SSADestructionPass> X("ssa-destruction",
                                          "Destructs the MemOIR SSA form.");

// Next there is code to register your pass to "clang"
static SSADestructionPass *_PassMaker = NULL;
static RegisterStandardPasses _RegPass1(
    PassManagerBuilder::EP_OptimizerLast,
    [](const PassManagerBuilder &, legacy::PassManagerBase &PM) {
      if (!_PassMaker) {
        PM.add(_PassMaker = new SSADestructionPass());
      }
    }); // ** for -Ox
static RegisterStandardPasses _RegPass2(
    PassManagerBuilder::EP_EnabledOnOptLevel0,
    [](const PassManagerBuilder &, legacy::PassManagerBase &PM) {
      if (!_PassMaker) {
        PM.add(_PassMaker = new SSADestructionPass());
      }
    }); // ** for -O0