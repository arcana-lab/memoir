#include "memoir/support/Assert.hpp"
#include "memoir/support/Casting.hpp"
#include "memoir/support/Print.hpp"

#include "SSADestruction.hpp"

namespace llvm::memoir {

SSADestructionVisitor::SSADestructionVisitor(llvm::noelle::DomTreeSummary &DT,
                                             SSADestructionStats *stats)
  : DT(DT),
    stats(stats) {
  // Do nothing.
}

void SSADestructionVisitor::visitInstruction(llvm::Instruction &I) {
  return;
}

void SSADestructionVisitor::visitUsePHIInst(UsePHIInst &I) {
  auto &used_collection = I.getUsedCollectionOperand();
  auto &collection = I.getCollectionValue();

  collection.replaceAllUsesWith(&used_collection);

  this->markForCleanup(I);

  return;
}

void SSADestructionVisitor::visitDefPHIInst(DefPHIInst &I) {
  auto &defined_collection = I.getDefinedCollectionOperand();
  auto &collection = I.getCollectionValue();

  collection.replaceAllUsesWith(&defined_collection);

  this->markForCleanup(I);

  return;
}

void SSADestructionVisitor::cleanup() {
  for (auto *inst : instructions_to_delete) {
    println(*inst);
    inst->eraseFromParent();
  }
}

void SSADestructionVisitor::markForCleanup(MemOIRInst &I) {
  this->markForCleanup(I.getCallInst());
}

void SSADestructionVisitor::markForCleanup(llvm::Instruction &I) {
  this->instructions_to_delete.insert(&I);
}

} // namespace llvm::memoir
