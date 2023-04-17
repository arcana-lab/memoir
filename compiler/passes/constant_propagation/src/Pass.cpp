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
#include "memoir/ir/InstVisitor.hpp"
#include "memoir/ir/Instructions.hpp"

// #include "memoir/analysis/AccessAnalysis.hpp"
#include "memoir/analysis/CollectionAnalysis.hpp"
#include "memoir/analysis/StructAnalysis.hpp"
#include "memoir/analysis/TypeAnalysis.hpp"

#include "memoir/support/InternalDatatypes.hpp"

#include "memoir/support/Print.hpp"
#include "memoir/utility/FunctionNames.hpp"
#include "memoir/utility/Metadata.hpp"

using namespace llvm::memoir;

namespace {

class ConstantPropagationVisitor
  : public llvm::memoir::InstVisitor<ConstantPropagationVisitor, void> {
public:
  void visitInstruction(llvm::Instruction &I) {
    println("  LLVM Instruction", I);
  }

  void visitMemOIRInst(MemOIRInst &I) {
    println("Memoir Instruction", I);
  }
};

struct ConstantPropagationPass : public ModulePass {
  static char ID;

  ConstantPropagationPass() : ModulePass(ID) {}

  bool doInitialization(Module &M) override {
    return false;
  }

  bool runOnModule(Module &M) override {
    errs() << "Running example pass\n\n";
    for (auto &F : M) {

      for (auto &BB : F) {
        for (auto &I : BB) {
          ConstantPropagationVisitor vis;
          vis.visit(&I);
        }
      }
    }

    return false;

    auto &noelle = getAnalysis<Noelle>();
    auto &type_analysis = TypeAnalysis::get();

    errs() << "=========================================\n";
    errs() << "Fetching all Collections\n\n";
    auto &CA = CollectionAnalysis::get(noelle);
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

            if (auto cllct = CollectionAnalysis::analyze(*call_inst)) {
              errs() << "Found collection for " << I << "\n";
              errs() << *cllct << "\n\n";
            }
          }
        }
      }
    }
    errs() << "=========================================\n\n";

    return false;
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<Noelle>();
    return;
  }
};

} // namespace

// Next there is code to register your pass to "opt"
char ConstantPropagationPass::ID = 0;
static RegisterPass<ConstantPropagationPass> X(
    "ConstantPropagation",
    "An example pass using the MemOIR analyses");

// Next there is code to register your pass to "clang"
static ConstantPropagationPass *_PassMaker = NULL;
static RegisterStandardPasses _RegPass1(
    PassManagerBuilder::EP_OptimizerLast,
    [](const PassManagerBuilder &, legacy::PassManagerBase &PM) {
      if (!_PassMaker) {
        PM.add(_PassMaker = new ConstantPropagationPass());
      }
    }); // ** for -Ox
static RegisterStandardPasses _RegPass2(
    PassManagerBuilder::EP_EnabledOnOptLevel0,
    [](const PassManagerBuilder &, legacy::PassManagerBase &PM) {
      if (!_PassMaker) {
        PM.add(_PassMaker = new ConstantPropagationPass());
      }
    }); // ** for -O0
