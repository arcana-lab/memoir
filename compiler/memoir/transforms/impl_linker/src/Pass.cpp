#include <iostream>
#include <string>

// LLVM
#include "llvm/Pass.h"

#include "llvm/IR/CFG.h"
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

#include "memoir/lowering/ImplLinker.hpp"
#include "memoir/lowering/TypeLayout.hpp"

using namespace llvm::memoir;

/*
 * This pass collects all collection implementations that will be needed for SSA
 * destruction and instantiates them.
 *
 * Author(s): Tommy McMichen
 * Created: February 19, 2024
 */

#define ASSOC_IMPL "stl_unordered_map"
#define SEQ_IMPL "stl_vector"

namespace llvm::memoir {

llvm::cl::opt<std::string> impl_file_output(
    "impl-out-file",
    llvm::cl::desc("Specify output filename for ImplLinker."),
    llvm::cl::value_desc("filename"));

struct ImplLinkerPass : public ModulePass {
  static char ID;

  ImplLinkerPass() : ModulePass(ID) {}

  bool doInitialization(Module &M) override {
    return false;
  }

  bool runOnModule(llvm::Module &M) override {
    TypeAnalysis::invalidate();

    // Get the TypeConverter.
    TypeConverter TC(M.getContext());

    // Get the ImplLinker.
    ImplLinker IL(M);

    for (auto &F : M) {
      if (F.empty()) {
        continue;
      }

      // Collect all of the collection allocations in the program.
      for (auto &BB : F) {
        for (auto &I : BB) {
          if (auto *seq_alloc = as<SequenceAllocInst>(&I)) {
            // Get the implementation name for this allocation.
            auto impl_name = SEQ_IMPL;

            // Get the type layout for the element type.
            auto &element_layout = TC.convert(seq_alloc->getElementType());

            // Implement the sequence.
            IL.implement_seq(impl_name, element_layout);

          } else if (auto *assoc_alloc = as<AssocAllocInst>(&I)) {
            // Get the implementation name for this allocation.
            auto impl_name = ASSOC_IMPL;

            // Get the type layout for the key type.
            auto &key_layout = TC.convert(assoc_alloc->getKeyType());

            // Get the type layout for the value type.
            auto &value_layout = TC.convert(assoc_alloc->getValueType());

            // Implement the assoc.
            IL.implement_assoc(impl_name, key_layout, value_layout);
          }
        }
      }
    }

    // Emit the implementation code.
    if (impl_file_output.getNumOccurrences()) {
      std::error_code error;
      llvm::raw_fd_ostream os(impl_file_output, error);
      if (error) {
        MEMOIR_UNREACHABLE("ImplLinker: Could not open output file!");
      }
      IL.emit(os);
    } else {
      IL.emit();
    }

    return false;
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    return;
  }
};

} // namespace llvm::memoir

// Next there is code to register your pass to "opt"
char ImplLinkerPass::ID = 0;
static RegisterPass<ImplLinkerPass> X(
    "memoir-impl-linker",
    "Instantiates the needed collection implementations.");
