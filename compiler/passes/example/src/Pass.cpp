#include <iostream>
#include <string>

#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include "noelle/core/Noelle.hpp"

#include "memoir/ir/Function.hpp"
#include "memoir/ir/Instructions.hpp"

// #include "memoir/analysis/AccessAnalysis.hpp"
// #include "memoir/analysis/AllocationAnalysis.hpp"
#include "memoir/analysis/TypeAnalysis.hpp"

#include "memoir/support/InternalDatatypes.hpp"

#include "memoir/utility/FunctionNames.hpp"
#include "memoir/utility/Metadata.hpp"

using namespace llvm::memoir;

namespace {

struct ExamplePass : public ModulePass {
  static char ID;

  ExamplePass() : ModulePass(ID) {}

  bool doInitialization(Module &M) override {
    return false;
  }

  bool runOnModule(Module &M) override {
    errs() << "Running example pass\n\n";

    auto &type_analysis = TypeAnalysis::get();
    // auto &allocation_analysis = AllocationAnalysis::get(M);
    // auto &access_analysis = AccessAnalysis::get(M);

    errs() << "Fetching all Type Summaries\n\n";
    for (auto &F : M) {
      if (memoir::MetadataManager::hasMetadata(F, MetadataType::INTERNAL)) {
        continue;
      }

      for (auto &BB : F) {
        for (auto &I : BB) {
          if (auto type = type_analysis.getType(I)) {
            errs() << "Found type for " << I << "\n";
            errs() << *type << "\n\n";
          } else {
            errs() << "Instruction does not have a MemOIR type.";
          }
        }
      }
    }

    // errs() << "Fetching all Allocation Summaries\n\n";
    // for (auto &F : M) {
    //   if (memoir::MetadataManager::hasMetadata(F, MetadataType::INTERNAL)) {
    //     continue;
    //   }

    //   for (auto &BB : F) {
    //     for (auto &I : BB) {
    //       if (auto call_inst = dyn_cast<CallInst>(&I)) {
    //         if (!FunctionNames::is_memoir_call(*call_inst)) {
    //           continue;
    //         }

    //         if (auto allocation_summary =
    //                 allocation_analysis.getAllocationSummary(*call_inst)) {
    //           errs() << "Found allocation summary for " << I << "\n";
    //           errs() << *allocation_summary << "\n\n";
    //         }
    //       }
    //     }
    //   }
    // }

    // errs() << "Fetching all Access Summaries\n\n";
    // for (auto &F : M) {
    //   if (memoir::MetadataManager::hasMetadata(F, MetadataType::INTERNAL)) {
    //     continue;
    //   }

    //   for (auto &BB : F) {
    //     for (auto &I : BB) {
    //       if (auto call_inst = dyn_cast<CallInst>(&I)) {
    //         if (!FunctionNames::is_memoir_call(*call_inst)) {
    //           continue;
    //         }

    //         if (auto access_summary = access_analysis.getAccessSummary(I)) {
    //           errs() << "Found access summary for " << I << "\n";
    //           errs() << *access_summary << "\n\n";
    //         }
    //       }
    //     }
    //   }
    // }

    return false;
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    return;
  }
};

} // namespace

// Next there is code to register your pass to "opt"
char ExamplePass::ID = 0;
static RegisterPass<ExamplePass> X("ExamplePass",
                                   "An example pass using the MemOIR analyses");

// Next there is code to register your pass to "clang"
static ExamplePass *_PassMaker = NULL;
static RegisterStandardPasses _RegPass1(PassManagerBuilder::EP_OptimizerLast,
                                        [](const PassManagerBuilder &,
                                           legacy::PassManagerBase &PM) {
                                          if (!_PassMaker) {
                                            PM.add(_PassMaker =
                                                       new ExamplePass());
                                          }
                                        }); // ** for -Ox
static RegisterStandardPasses _RegPass2(
    PassManagerBuilder::EP_EnabledOnOptLevel0,
    [](const PassManagerBuilder &, legacy::PassManagerBase &PM) {
      if (!_PassMaker) {
        PM.add(_PassMaker = new ExamplePass());
      }
    }); // ** for -O0
