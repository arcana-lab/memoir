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
#include "memoir/analysis/StructAnalysis.hpp"
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

    errs() << "=========================================\n";
    errs() << "Fetching all Types\n\n";
    for (auto &F : M) {
      if (memoir::MetadataManager::hasMetadata(F, MetadataType::INTERNAL)) {
        continue;
      }

      for (auto &BB : F) {
        for (auto &I : BB) {
          if (auto type = TypeAnalysis::analyze(I)) {
            errs() << "Found type for " << I << "\n";
            errs() << *type << "\n\n";
          }
        }
      }
    }
    errs() << "=========================================\n\n";

    errs() << "=========================================\n";
    errs() << "Fetching all Structs\n\n";
    for (auto &F : M) {
      if (memoir::MetadataManager::hasMetadata(F, MetadataType::INTERNAL)) {
        continue;
      }

      for (auto &BB : F) {
        for (auto &I : BB) {
          if (auto call_inst = dyn_cast<CallInst>(&I)) {
            if (!FunctionNames::is_memoir_call(*call_inst)) {
              continue;
            }

            if (auto strct = StructAnalysis::analyze(*call_inst)) {
              errs() << "Found struct for " << I << "\n";
              errs() << *strct << "\n\n";
            }
          }
        }
      }
    }
    errs() << "=========================================\n\n";

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
