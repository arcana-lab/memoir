/*
 * Pass to perform constant propagation on MemOIR collections.
 *
 * Author: Nick Wanninger
 * Created: April 17, 2023
 */

#include "ConstantPropagation.hpp"
#include "memoir/ir/InstVisitor.hpp"
#include "memoir/support/Print.hpp"

using namespace llvm::memoir;

class ConstPropVisitor
  : public llvm::memoir::InstVisitor<ConstPropVisitor, void> {
  friend class llvm::memoir::InstVisitor<ConstPropVisitor, void>;
  friend class llvm::InstVisitor<ConstPropVisitor, void>;

public:
  void visitInstruction(llvm::Instruction &I) {
    // println("Memoir Inst: ", I);
  }
  void visitMemOIRInst(MemOIRInst &I) {
    println("Memoir Inst: ", I);
  }
};

namespace constprop {

ConstantPropagation::ConstantPropagation(Module &M, Noelle &noelle)
  : M(M),
    noelle(noelle) {
  // Do initialization.
}

void ConstantPropagation::analyze() {
  // Analyze the program
  println("Constant Prop: Analyze");
  ConstPropVisitor vis;

  for (auto &F : M) {
    for (auto &BB : F) {
      for (auto &I : BB) {
        vis.visit(&I);
      }
    }
  }

  return;
}

void ConstantPropagation::transform() {
  // Transform the program
  println("Constant Prop: Transform");

  return;
}

} // namespace constprop
