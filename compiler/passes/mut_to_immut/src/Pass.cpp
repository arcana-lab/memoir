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

#include "memoir/analysis/TypeAnalysis.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/support/InternalDatatypes.hpp"
#include "memoir/support/Print.hpp"
#include "memoir/support/Timer.hpp"

#include "memoir/utility/FunctionNames.hpp"
#include "memoir/utility/Metadata.hpp"

#include "MutToImmutVisitor.hpp"

using namespace llvm::memoir;

/*
 * This pass converts operations on mutable collections to operations on
 * immutable collections.
 *
 * Author(s): Tommy McMichen
 * Created: July 26, 2023
 */

namespace llvm::memoir {

struct MutToImmutPass : public ModulePass {
  static char ID;

  MutToImmutPass() : ModulePass(ID) {}

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

    return traversal;
  }

  static DomTreeTraversalListTy dfs_preorder_traversal(
      llvm::noelle::DomTreeSummary &DT,
      llvm::BasicBlock &root) {
    auto *root_node = DT.getNode(&root);
    MEMOIR_NULL_CHECK(root_node, "Root node couldn't be found, blame NOELLE");

    return dfs_preorder_traversal_helper(root_node);
  }

  bool runOnModule(Module &M) override {
    infoln();
    infoln("BEGIN mut2immut pass");
    infoln();

    TypeAnalysis::invalidate();

    // Get NOELLE.
    auto &NOELLE = getAnalysis<llvm::noelle::Noelle>();

    MutToImmutStats stats;

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

      // Get the dominance frontier.
      auto &DF = getAnalysis<llvm::DominanceFrontierWrapperPass>(F)
                     .getDominanceFrontier();

      // Get the depth-first, preorder traversal of the dominator tree rooted at
      // the entry basic block.
      auto &entry_bb = F.getEntryBlock();
      auto dfs_traversal = dfs_preorder_traversal(DT, entry_bb);

      // Collect all source-level collection pointers names.
      ordered_set<llvm::Value *> memoir_names = {};
      for (auto &A : F.args()) {
        if (Type::value_is_collection_type(A)) {
          memoir_names.insert(&A);
        }
      }
      for (auto &BB : F) {
        for (auto &I : BB) {
          if (Type::value_is_collection_type(I)) {
            memoir_names.insert(&I);
          }
        }
      }

      // Insert PHIs.
      map<llvm::PHINode *, llvm::Value *> inserted_phis = {};
      for (auto *name : memoir_names) {
        // Get information about the named variable.
        auto *type = name->getType();

        // Gather the set of basic blocks containing definitions of the
        // named variable.
        set<llvm::BasicBlock *> def_parents = {};
        queue<llvm::BasicBlock *> def_parents_worklist = {};
        if (auto *name_as_inst = dyn_cast<llvm::Instruction>(name)) {
          if (auto *name_bb = name_as_inst->getParent()) {
            def_parents.insert(name_bb);
            def_parents_worklist.push(name_bb);
          }
        }

        // Gather the set of basic blocks containing mutators and PHI nodes.
        for (auto *def : name->users()) {
          if (auto *def_as_inst = dyn_cast<llvm::Instruction>(def)) {
            auto *memoir_inst = MemOIRInst::get(*def_as_inst);
            if (!memoir_inst) {
              continue;
            }
            // Add check if this is a mutator
            if (!isa<MutInst>(memoir_inst) && !isa<AccessInst>(memoir_inst)) {
              continue;
            }
            if (auto *append_inst = dyn_cast<MutSeqAppendInst>(memoir_inst)) {
              if (name != &append_inst->getCollection()) {
                continue;
              }
            }
            if (auto *def_bb = def_as_inst->getParent()) {
              debugln("inserting def parent for ", *def_as_inst);
              def_parents.insert(def_bb);
              def_parents_worklist.push(def_bb);
            }
          }
        }

        debugln("Def parents:");
        for (auto *def_parent : def_parents) {
          debugln(*def_parent);
        }

        debugln("inserting PHIs");
        // Gather the set of basic blocks where PHIs need to be added.
        set<llvm::BasicBlock *> phi_parents = {};
        while (!def_parents_worklist.empty()) {
          // Pop the basic block off the set.
          auto *bb = def_parents_worklist.front();
          def_parents_worklist.pop();

          // For each basic block in the dominance frontier.
          auto found_dom_frontier = DF.find(bb);
          MEMOIR_ASSERT(found_dom_frontier != DF.end(),
                        "Couldn't find dominance frontier for basic block.");
          auto &dominance_frontier = found_dom_frontier->second;
          for (auto *frontier_bb : dominance_frontier) {
            if (phi_parents.find(frontier_bb) == phi_parents.end()) {
              // If the name doesn't dominate all of the predecessors, don't
              // insert a PHI node.
              auto name_doesnt_dominate = false;
              if (auto *name_as_inst = dyn_cast<llvm::Instruction>(name)) {
                for (auto *pred_bb : llvm::predecessors(frontier_bb)) {
                  auto *name_bb = name_as_inst->getParent();
                  if (!DT.dominates(name_bb, pred_bb)) {
                    name_doesnt_dominate = true;
                  }
                }
              }
              if (name_doesnt_dominate) {
                continue;
              }

              // If name is already used in one of the PHI nodes, don't insert a
              // PHI node.
              bool name_already_in_phi = false;
              for (auto &phi : frontier_bb->phis()) {
                for (auto &incoming : phi.incoming_values()) {
                  if (incoming.get() == name) {
                    name_already_in_phi = true;
                    break;
                  }
                }
              }
              if (name_already_in_phi) {
                continue;
              }

              // Insert a PHI node at the beginning of this basic block.
              IRBuilder<> phi_builder(frontier_bb->getFirstNonPHI());
              auto num_incoming_edges = llvm::pred_size(frontier_bb);
              auto *phi = phi_builder.CreatePHI(type, num_incoming_edges);
              MEMOIR_NULL_CHECK(phi,
                                "Couldn't create PHI at dominance frontier");
              debugln("Inserting PHI: ", *phi);

              // Insert a dummy value for the time being.
              for (auto *pred_bb : llvm::predecessors(frontier_bb)) {
                phi->addIncoming(name, pred_bb);
              }

              // Register the PHI with the name its being inserted for.
              inserted_phis[phi] = name;

              // Insert the basic block into the PHI set.
              phi_parents.insert(frontier_bb);

              // Insert the basic block into the Def set.
              // TODO: this may need to check for set residency
              def_parents_worklist.push(frontier_bb);
            }
          }
        }
      }

      // Initialize the reaching definitions.
      MutToImmutVisitor MTIV(DT, memoir_names, inserted_phis, &stats);

      // Apply rewrite rules and renaming for reaching definitions.
      infoln("Applying rewrite rules");
      for (auto *bb : dfs_traversal) {
        for (auto &I : *bb) {
          MTIV.visit(I);
        }

        // Update PHIs with the reaching definition.
        debugln("Updating immediate successors");
        debugln(*bb);
        for (auto *succ_bb : llvm::successors(bb)) {
          for (auto &phi : succ_bb->phis()) {
            // Ensure that the value is of collection type.
            if (!Type::value_is_collection_type(phi)) {
              continue;
            }

            debugln("Updating successor PHI: ");
            debugln(phi);

            auto &incoming_use = phi.getOperandUse(phi.getBasicBlockIndex(bb));
            auto *incoming_value = incoming_use.get();

            // Update the reaching definition for the incoming edge.
            auto *reaching_definition =
                MTIV.update_reaching_definition(incoming_value,
                                                bb->getTerminator());
            incoming_use.set(reaching_definition);
          }
        }
      }

      infoln("Cleaning up dead mutable instructions.");
      MTIV.cleanup();

      infoln("END: ", F.getName());
      infoln("=========================");
    }

    infoln("=========================");
    infoln("DONE mut2immut pass");

    infoln();

    TypeAnalysis::invalidate();
    MemOIRInst::invalidate();

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
char MutToImmutPass::ID = 0;
static RegisterPass<MutToImmutPass> X(
    "mut2immut",
    "Converts mutable collection operations to their immutable, SSA form.");

// Next there is code to register your pass to "clang"
static MutToImmutPass *_PassMaker = NULL;
static RegisterStandardPasses _RegPass1(PassManagerBuilder::EP_OptimizerLast,
                                        [](const PassManagerBuilder &,
                                           legacy::PassManagerBase &PM) {
                                          if (!_PassMaker) {
                                            PM.add(_PassMaker =
                                                       new MutToImmutPass());
                                          }
                                        }); // ** for -Ox
static RegisterStandardPasses _RegPass2(
    PassManagerBuilder::EP_EnabledOnOptLevel0,
    [](const PassManagerBuilder &, legacy::PassManagerBase &PM) {
      if (!_PassMaker) {
        PM.add(_PassMaker = new MutToImmutPass());
      }
    }); // ** for -O0
