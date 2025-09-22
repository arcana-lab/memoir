#include <string>

#include "llvm/IR/Function.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"

#include "memoir/passes/Passes.hpp"

#include "memoir/ir/InstVisitor.hpp"
#include "memoir/ir/Instructions.hpp"

#include "memoir/support/InternalDatatypes.hpp"

#include "memoir/utility/FunctionNames.hpp"
#include "memoir/utility/Metadata.hpp"

/*
 * This file contains an implementation of the pass from "Writing a Pass" in the
 * MEMOIR User Guide.
 *
 * Author(s): Tommy McMichen
 */

using namespace memoir;

namespace {

class MyVisitor : public memoir::InstVisitor<MyVisitor, void> {
  // In order for the wrapper to work, we need to declare our parent classes as
  // friends.
  friend class memoir::InstVisitor<MyVisitor, void>;
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
  void visitAccessInst(memoir::AccessInst &I) {
    this->instruction_counts["access"]++;
    return;
  }

  // Let's do the same for allocation instructions:
  void visitAllocInst(memoir::AllocInst &I) {
    this->instruction_counts["alloc"]++;
    return;
  }

  // Put everything else into an "other" bucket.
  // NOTE: since visitAllocInst and visitAccessInst are
  //       implemented, visitMemOIRInst will _never_ be
  //       passed an AllocInst nor an AccessInst.
  void visitMemOIRInst(memoir::MemOIRInst &I) {
    this->instruction_counts["other"]++;
    return;
  }
};

} // namespace

namespace memoir {

llvm::PreservedAnalyses ExamplePass::run(llvm::Module &M,
                                         llvm::ModuleAnalysisManager &MAM) {
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
    memoir::println(type, " -> ", count, "\n");
  }

  // We did not modify the program, so all analyses are preserved
  return llvm::PreservedAnalyses::all();
}

} // namespace memoir
