// LLVM
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include "llvm/Analysis/CallGraph.h"

// NOELLE
#include "noelle/core/Noelle.hpp"

// Type Inference
#include "TypeInference.hpp"

using namespace arcana::noelle;

namespace llvm::memoir {

/*
 * This pass performs type inference on MEMOIR variables, adding explicit type
 * annotations where necessary.
 *
 * Author(s): Tommy McMichen
 * Created: December 19, 2023
 */

struct TypeInferencePass : public ModulePass {
  static char ID;

  TypeInferencePass() : ModulePass(ID) {}

  bool doInitialization(llvm::Module &M) override {
    return false;
  }

  bool runOnModule(llvm::Module &M) override {

    TypeAnalysis::invalidate();

    auto type_inference = new TypeInference(M);

    return type_inference->run();
  }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    return;
  }
};

} // namespace llvm::memoir

// Next there is code to register your pass to "opt"
char llvm::memoir::TypeInferencePass::ID = 0;
static RegisterPass<llvm::memoir::TypeInferencePass> X(
    "memoir-type-infer",
    "Infers types of MemOIR variables and adds type annotations.");
