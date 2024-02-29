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

#include "memoir/ir/Instructions.hpp"

#include "memoir/analysis/TypeAnalysis.hpp"

#include "memoir/support/InternalDatatypes.hpp"

#include "memoir/utility/FunctionNames.hpp"
#include "memoir/utility/Metadata.hpp"

/*
 * This file contains an implementation of the pass from "Writing a Pass" in the
 * MEMOIR User Guide.
 *
 * Author(s): Tommy McMichen
 */

namespace {

class MyVisitor : public llvm::memoir::InstVisitor<MyVisitor, void> {
  // In order for the wrapper to work, we need to declare our parent classes as
  // friends.
  friend class llvm::memoir::InstVisitor<MyVisitor, void>;
  friend class llvm::InstVisitor<MyVisitor, void>;

public:
  // We will store the results of our analysis here:
  std::map<std::string, uint32_t> instruction_counts;

  // We _always_ need to implement visitInstruction!
  void visitInstruction(llvm::Instruction &I) {
    // Do nothing.
    return;
  }

  // Count all access instructions (read, write, get) together:
  void visitAccessInst(llvm::memoir::AccessInst &I) {
    this->instruction_counts["access"]++;
    return;
  }

  // Let's do the same for allocation instructions:
  void visitAllocInst(llvm::memoir::AllocInst &I) {
    this->instruction_counts["alloc"]++;
    return;
  }

  // Put everything else into an "other" bucket.
  // NOTE: since visitAllocInst and visitAccessInst are
  //       implemented, visitMemOIRInst will _never_ be
  //       passed an AllocInst nor an AccessInst.
  void visitMemOIRInst(llvm::memoir::MemOIRInst &I) {
    this->instruction_counts["other"]++;
    return;
  }
};

struct ExamplePass : public llvm::ModulePass {
  static char ID;

  ExamplePass() : ModulePass(ID) {}

  bool doInitialization(llvm::Module &M) override {
    return false;
  }

  bool runOnModule(llvm::Module &M) override {
    // Initialize our visitor:
    MyVisitor visitor;

    // Analyze the program.
    for (llvm::Function &F : M) {
      for (llvm::BasicBlock &BB : F) {
        for (llvm::Instruction &I : BB) {
          visitor.visit(I);
        }
      }
    }

    // Print the results of our visitor:
    for (const auto &[type, count] : visitor.instruction_counts) {
      llvm::memoir::println(type, " -> ", count, "\n");
    }

    // We did not modify the program, so we return false.
    return false;
  }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    return;
  }
};

// Next there is code to register your pass to "opt"
char ExamplePass::ID = 0;
static llvm::RegisterPass<ExamplePass> X(
    "memoir-example",
    "An example pass using the MemOIR analyses");
} // namespace
