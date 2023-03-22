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
#include "memoir/ir/InstVisitor.hpp"
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

struct SlicePropagationPass : public ModulePass {
  static char ID;

  SlicePropagationPass() : ModulePass(ID) {}

  bool doInitialization(Module &M) override {
    return false;
  }

  bool runOnModule(Module &M) override {
    println("Running example pass");
    println();

    // Get the required analyses.
    auto &noelle = getAnalysis<llvm::noelle::Noelle>();
    auto &TA = TypeAnalysis::get();
    auto &SA = StructAnalysis::get();
    auto &CA = CollectionAnalysis::get(noelle);

    // auto loops = noelle.getLoops();
    // MEMOIR_NULL_CHECK(loops, "Unable to get the loops from NOELLE!");

    // Gather all memoir slices from each LLVM Function.
    map<llvm::Function *, set<SliceInst *>> slice_instructions = {};
    for (auto &F : M) {
      for (auto &BB : F) {
        for (auto &I : BB) {
          if (auto memoir_inst = MemOIRInst::get(I)) {
            if (auto slice_inst = dyn_cast<SliceInst>(memoir_inst)) {
              slice_instructions[&F].insert(slice_inst);
            }
          }
        }
      }
    }

    // Perform a backwards flow analysis to slice the collection as early as
    // possible.
    map<llvm::Value *, pair<llvm::Value *, llvm::Value *>>
        collections_to_slice = {};
    for (auto const &[func, slice_insts] : slice_instructions) {
      for (auto slice_inst : slice_insts) {
        // Get the LLVM representation of this slice instruction.
        auto &llvm_slice_inst = slice_inst->getCallInst();

        // Check that the sliced operand is only used by this slice instruction.
        // TODO: extend this to allow for multiple slices to be propagated.
        bool slice_is_only_user = true;
        auto &sliced_operand = slice_inst->getCollectionOperand();
        for (auto &use : sliced_operand.uses()) {
          auto user = use.getUser();
          if (user != &llvm_slice_inst) {
            slice_is_only_user = false;
            break;
          }
        }

        if (!slice_is_only_user) {
          continue;
        }

        // Look at the sliced collection to see if it can be sliced.
        auto &sliced_collection = slice_inst->getCollection();

        // If the sliced collection is returned from a function call, let's go
        // there.
        if (auto ret_phi = dyn_cast<RetPHICollection>(&sliced_collection)) {
        }
      }
    }

    return false;
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<llvm::noelle::Noelle>();
    AU.addRequired<llvm::DominatorTreeWrapperPass>();
    return;
  }
}; // namespace llvm::memoir

} // namespace llvm::memoir

// Next there is code to register your pass to "opt"
char SlicePropagationPass::ID = 0;
static RegisterPass<SlicePropagationPass> X(
    "SlicePropagation",
    "Propagates slices of collections used with loops of the program.");

// Next there is code to register your pass to "clang"
static SlicePropagationPass *_PassMaker = NULL;
static RegisterStandardPasses _RegPass1(
    PassManagerBuilder::EP_OptimizerLast,
    [](const PassManagerBuilder &, legacy::PassManagerBase &PM) {
      if (!_PassMaker) {
        PM.add(_PassMaker = new SlicePropagationPass());
      }
    }); // ** for -Ox
static RegisterStandardPasses _RegPass2(
    PassManagerBuilder::EP_EnabledOnOptLevel0,
    [](const PassManagerBuilder &, legacy::PassManagerBase &PM) {
      if (!_PassMaker) {
        PM.add(_PassMaker = new SlicePropagationPass());
      }
    }); // ** for -O0
