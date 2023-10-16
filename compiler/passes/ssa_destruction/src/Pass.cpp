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

  static DomTreeTraversalListTy dfs_postorder_traversal_helper(
      llvm::noelle::DomNodeSummary *root) {
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
      llvm::noelle::DomTreeSummary &DT,
      llvm::BasicBlock &root) {
    auto *root_node = DT.getNode(&root);
    MEMOIR_NULL_CHECK(root_node, "Root node couldn't be found, blame NOELLE");

    return std::move(dfs_postorder_traversal_helper(root_node));
  }

  bool runOnModule(llvm::Module &M) override {
    println("BEGIN SSA Destruction pass");
    println();

    // Get NOELLE.
    auto &NOELLE = getAnalysis<llvm::noelle::Noelle>();

    auto &CA = CollectionAnalysis::get(NOELLE);

    SSADestructionStats stats;

    // Initialize the reaching definitions.
    SSADestructionVisitor SSADV(M, &stats);

    for (auto &F : M) {
      if (F.empty()) {
        continue;
      }

      bool no_memoir = true;
      for (auto &A : F.args()) {
        if (Type::value_is_collection_type(A)
            || Type::value_is_struct_type(A)) {
          no_memoir = false;
          break;
        }
      }
      if (no_memoir) {
        for (auto &I : llvm::instructions(F)) {
          if (Type::value_is_collection_type(I) || Type::value_is_struct_type(I)
              || Type::value_is_type(I)) {
            no_memoir = false;
            break;
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
      auto &DT = NOELLE.getDominators(&F)->DT;

      // Compute the liveness analysis.
      auto LA = LivenessAnalysis(F, NOELLE.getDataFlowEngine());

      // Get a new value numbering instance.
      auto VN = ValueNumbering(M);

      // Hand over this function's analyses.
      SSADV.setAnalyses(DT, LA, VN);

      // Get the depth-first, preorder traversal of the dominator tree rooted at
      // the entry basic block.
      auto &entry_bb = F.getEntryBlock();
      auto dfs_preorder = dfs_preorder_traversal(DT, entry_bb);

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
    // auto dfs_postorder = dfs_postorder_traversal(DT, entry_bb);
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

    println("=========================");
    println("DONE SSA Destruction pass");
    println();

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
