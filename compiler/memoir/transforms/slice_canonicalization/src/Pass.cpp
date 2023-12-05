#include <iostream>
#include <string>

// LLVM
#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include "llvm/IR/Dominators.h"

#include "llvm/Analysis/CFG.h"

// NOELLE
#include "noelle/core/Noelle.hpp"

// MemOIR
#include "memoir/ir/Builder.hpp"
#include "memoir/ir/Function.hpp"
#include "memoir/ir/Instructions.hpp"

#include "memoir/analysis/CollectionAnalysis.hpp"
#include "memoir/analysis/StructAnalysis.hpp"
#include "memoir/analysis/TypeAnalysis.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/support/InternalDatatypes.hpp"
#include "memoir/support/Print.hpp"

#include "memoir/utility/FunctionNames.hpp"
#include "memoir/utility/Metadata.hpp"

using namespace llvm::memoir;

/*
 * This pass canonicalizes the conservative iteration space of sequences in
 * loops by inserting slice and join operations before and after the loop.
 *
 * Author(s): Tommy McMichen
 * Created: February 22, 2023
 */

namespace llvm::memoir {

struct SliceCanonicalizationPass : public ModulePass {
  static char ID;

  SliceCanonicalizationPass() : ModulePass(ID) {}

  bool doInitialization(Module &M) override {
    return false;
  }

  bool runOnModule(Module &M) override {
    println("Running Slice Canonicalization pass");
    println();

    // Get the required analyses.
    auto &noelle = getAnalysis<arcana::noelle::Noelle>();
    auto &TA = TypeAnalysis::get();
    auto &SA = StructAnalysis::get();
    auto &CA = CollectionAnalysis::get(noelle);

    auto loops = noelle.getLoops();
    MEMOIR_NULL_CHECK(loops, "Unable to get the loops from NOELLE!");

    // Gather all memoir collections and their canonical ranges.
    for (auto loop : *loops) {
      MEMOIR_NULL_CHECK(loop, "NOELLE gave us a NULL LoopDependenceInfo");

      auto loop_structure = loop->getLoopStructure();
      MEMOIR_NULL_CHECK(loop_structure, "NOELLE gave us a NULL LoopStructure");

      println();
      println("Found loop");

      set<MemOIRInst *> memoir_insts = {};
      for (auto inst : loop_structure->getInstructions()) {
        MEMOIR_NULL_CHECK(inst, "Instruction within loop structure is NULL!");
        if (auto memoir_inst = MemOIRInst::get(*inst)) {
          memoir_insts.insert(memoir_inst);
          println("  ", *memoir_inst);
        }
      }

      // If there are no memoir instructions, skip this loop.
      if (memoir_insts.empty()) {
        continue;
      }

      // Get the memoir index access instructions.
      set<AccessInst *> access_insts = {};
      for (auto memoir_inst : memoir_insts) {
        if (auto access_inst = dyn_cast<AccessInst>(memoir_inst)) {
          access_insts.insert(access_inst);
        }
      }

      // Get the subset of memoir accesses to loop invariant collections.
      map<llvm::Value *, set<AccessInst *>> loop_invariant_accesses = {};
      for (auto access_inst : access_insts) {
        // Check that this collection is sliceable (i.e. a sequence).
        auto &collection_type = access_inst->getCollectionType();
        if (!isa<SequenceType>(&collection_type)) {
          continue;
        }

        // Get the LLVM value for the collection.
        auto &collection = access_inst->getObjectOperand();

        // Check that this collection is loop invariant.
        // TODO: add support for loop variant collections with swap SCC's
        if (loop_structure->isLoopInvariant(&collection)) {
          println("Found access to loop invariant collection");
          println("  access: ", *access_inst);
          println("  collection: ", collection);

          loop_invariant_accesses[&collection].insert(access_inst);
        }
      }

      // Get this loop's IV Manager.
      auto ivm = loop->getInductionVariableManager();
      MEMOIR_NULL_CHECK(ivm, "NOELLE gave us a NULL InductionVariableManager");

      // For each loop invariant collection access, check that each access
      // is to the loop-governing induction variable.
      auto loop_governing_iv_attr =
          ivm->getLoopGoverningIVAttribution(*loop_structure);
      if (loop_governing_iv_attr == nullptr) {
        println("Could not determine the loop governing "
                "induction variable for this loop!");
        continue;
      }

      auto &loop_governing_iv = loop_governing_iv_attr->getInductionVariable();
      auto exit_value = loop_governing_iv_attr->getExitConditionValue();
      if (exit_value == nullptr) {
        println("Could not determine the exit condition value");
        continue;
      }

      auto start_value = loop_governing_iv.getStartValue();
      if (start_value == nullptr) {
        println("Could not determine the start value");
        continue;
      }

      auto step_value = loop_governing_iv.getSingleComputedStepValue();
      if (step_value == nullptr) {
        println("Could not determine the step value");
        continue;
      }

      // Ensure that the step value is a constant 1.
      // TODO: make this less conservative with stencils.
      auto step_constant = dyn_cast<llvm::ConstantInt>(step_value);
      if (step_constant == nullptr) {
        println("Step value is not a constant");
        continue;
      }
      if (step_constant->getZExtValue() != 1) {
        println("Step value is not 1");
        continue;
      }

      // Get the dominator tree for the function containing this loop.
      auto func = loop_structure->getFunction();
      auto &DTA = getAnalysis<DominatorTreeWrapperPass>(*func);
      auto const &DT = DTA.getDomTree();

      // Find accesses to loop invariant collections that can be marked for
      // slice canonicalization.
      for (auto const &[collection, accesses] : loop_invariant_accesses) {
        bool is_governed = true;
        for (auto access_inst : accesses) {
          // Ensure that this access is for a single dimension and determine the
          // index operand, so long as all collection accesses are indexed.
          llvm::Value *index_operand;
          if (auto index_read_inst = dyn_cast<IndexReadInst>(access_inst)) {
            if (index_read_inst->getNumberOfDimensions() != 1) {
              println("Access has more than one dimension.");
              is_governed = false;
              break;
            }

            index_operand = &(index_read_inst->getIndexOfDimension(0));
          } else if (auto index_write_inst =
                         dyn_cast<IndexWriteInst>(access_inst)) {
            if (index_write_inst->getNumberOfDimensions() != 1) {
              println("Access has more than one dimension.");
              is_governed = false;
              break;
            }

            index_operand = &(index_write_inst->getIndexOfDimension(0));
          } else if (auto index_get_inst =
                         dyn_cast<IndexGetInst>(access_inst)) {
            if (index_get_inst->getNumberOfDimensions() != 1) {
              println("Access has more than one dimension.");
              is_governed = false;
              break;
            }

            index_operand = &(index_get_inst->getIndexOfDimension(0));
          } else {
            println("Access to collection is not indexed");
            is_governed = false;
            break;
          }

          // See if the index operand is an IV instruction, if it's not, then
          // accesses to this collection are not governed.
          if (auto index_as_inst = dyn_cast<llvm::Instruction>(index_operand)) {
            if (!loop_governing_iv.isIVInstruction(index_as_inst)) {
              println(
                  "Index is not the loop governing IV, consider adding SCEV");
              is_governed = false;
              break;
            }
          } else {
            println("Index is not an instruction, unsupported");
            is_governed = false;
            break;
          }
        }

        // If accesses to this collection are not governed by the loop governing
        // IV, then continue to the next collection accessed in this loop.
        if (!is_governed) {
          println("Collection is not governed by the loop governing IV");
          continue;
        }

        // If the loop governing IV has the same bounds as the collection being
        // accessed, don't perform slicing.
        // TODO:

        // Get the preheader of the loop.
        auto preheader_bb = loop_structure->getPreHeader();
        MEMOIR_NULL_CHECK(preheader_bb,
                          "Could not determine the preheader of the loop!");

        // Split the preheader, keeping all PHINodes in the new preheader.
        // auto post_preheader_bb =
        //     llvm::SplitBlock(preheader_bb,
        //     preheader_bb->getFirstNonPHIOrDbg());

        // Initialize a MemOIRBuilder at the preheader.
        MemOIRBuilder builder(preheader_bb->getTerminator());

        // Create the slices for this collection's range.
        // NOTE: this will probably not work with nested loops.
        // NOTE: may want to improve this for Collections that are PHINodes in
        //       the preheader.
        auto end_constant = builder.getInt64(-1);
        auto live_slice_inst = builder.CreateSliceInst(collection,
                                                       start_value,
                                                       exit_value,
                                                       "live.");
        SliceInst *rest_slice_inst = nullptr;

        // Replace uses of the collection withing the loop with the live slice.
        for (auto &use : collection->uses()) {
          // Check that user is an instruction.
          auto user_as_inst = dyn_cast<llvm::Instruction>(use.getUser());
          if (!user_as_inst) {
            continue;
          }

          // Check that user is in the loop.
          if (!loop_structure->isIncluded(user_as_inst)) {
            continue;
          }

          // Replace use of the collection with the live slice.
          use.set(&(live_slice_inst->getCallInst()));
        }

        // Create the loop-closing join for this collection's canonical slices
        // in each exit block.

        // TODO: Add PHIs for the case where we have multiple exit blocks.
        //       Use the dominance frontier of the exit basic blocks to do this.
        auto loop_exits = loop_structure->getLoopExitBasicBlocks();
        if (loop_exits.size() > 1) {
          println("Loop has more than one exit basic block!");
          println("Transformations for these loops is currently UNSUPPORTED");
          continue;
        }

        for (auto exit_bb : loop_exits) {
          // Check that there is a use of the collection reachable from the exit
          // basic block.
          set<llvm::Use *> reachable_uses = {};
          for (auto &use : collection->uses()) {
            auto user = use.getUser();
            auto user_as_inst = dyn_cast<llvm::Instruction>(user);
            if (!user_as_inst) {
              continue;
            }

            // Check if the user's basic block parent is reachable from the exit
            // basic block.
            auto user_bb = user_as_inst->getParent();
            if (llvm::isPotentiallyReachable(exit_bb, user_bb, &DT)) {
              reachable_uses.insert(&use);
            }
          }

          // If there are no reachable uses, don't generate the join inst.
          if (reachable_uses.empty()) {
            continue;
          }

          // If the rest slice has not been created yet, create it.
          if (!rest_slice_inst) {
            builder.SetInsertPoint(preheader_bb->getTerminator());
            rest_slice_inst = builder.CreateSliceInst(collection,
                                                      exit_value,
                                                      end_constant,
                                                      "rest.");
          }

          // Move the builder's insertion point to the exit basic block, and
          // build the list of slices to join.
          builder.SetInsertPoint(exit_bb->getTerminator());
          vector<llvm::Value *> slices_to_join = {
            &(live_slice_inst->getCallInst()),
            &(rest_slice_inst->getCallInst())
          };

          // Create the JoinInst for this exit basic block.
          auto join_inst = builder.CreateJoinInst(slices_to_join, "lcjoin.");
          auto llvm_join_inst = &(join_inst->getCallInst());

          // Replace uses of the collection reachable from and dominated by the
          // exit basic block.
          for (auto use : reachable_uses) {
            // Check that user is an instruction.
            auto user = use->getUser();
            auto user_as_inst = dyn_cast<llvm::Instruction>(user);
            if (!user_as_inst) {
              continue;
            }

            // Check that user is dominated by the join inst.
            if (DT.dominates(llvm_join_inst, *use)) {
              use->set(llvm_join_inst);
            }
          }
        }
      }
    }

    return false;
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<arcana::noelle::Noelle>();
    AU.addRequired<llvm::DominatorTreeWrapperPass>();
    return;
  }
};

} // namespace llvm::memoir

// Next there is code to register your pass to "opt"
char SliceCanonicalizationPass::ID = 0;
static RegisterPass<SliceCanonicalizationPass> X(
    "SliceCanonicalization",
    "Canonicalizes the slices of collections used with loops of the program.");

// Next there is code to register your pass to "clang"
static SliceCanonicalizationPass *_PassMaker = NULL;
static RegisterStandardPasses _RegPass1(
    PassManagerBuilder::EP_OptimizerLast,
    [](const PassManagerBuilder &, legacy::PassManagerBase &PM) {
      if (!_PassMaker) {
        PM.add(_PassMaker = new SliceCanonicalizationPass());
      }
    }); // ** for -Ox
static RegisterStandardPasses _RegPass2(
    PassManagerBuilder::EP_EnabledOnOptLevel0,
    [](const PassManagerBuilder &, legacy::PassManagerBase &PM) {
      if (!_PassMaker) {
        PM.add(_PassMaker = new SliceCanonicalizationPass());
      }
    }); // ** for -O0
