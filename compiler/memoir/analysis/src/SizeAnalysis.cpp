// LLVM

// MemOIR
#include "memoir/analysis/SizeAnalysis.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/support/Print.hpp"

namespace llvm::memoir {
SizeAnalysis::~SizeAnalysis() {
  // Do nothing.
}

// Top-level analysis driver.
ValueExpression *SizeAnalysis::getSize(llvm::Value &V) {
  // Visit the collection.
  if (auto *inst = dyn_cast<llvm::Instruction>(&V)) {
    return this->visit(*inst);
  }

  else if (auto *arg = dyn_cast<llvm::Argument>(&V)) {
    return this->visitArgument(*arg);
  }

  return nullptr;
}

// Analysis visitors.
ValueExpression *SizeAnalysis::visitArgument(llvm::Argument &I) {
  return nullptr;
}

ValueExpression *SizeAnalysis::visitInstruction(llvm::Instruction &I) {
  return nullptr;
}

ValueExpression *SizeAnalysis::visitSequenceAllocInst(SequenceAllocInst &I) {
  // Get the size operand.
  auto &size_operand = I.getSizeOperand();

  // Get the ValueExpression for it.
  auto *size_expr = this->VN.get(size_operand);

  return size_expr;
}

ValueExpression *SizeAnalysis::visitPHINode(llvm::PHINode &I) {
  // Visit each of the incoming edges.
  vector<ValueExpression *> incoming_expressions = {};
  vector<llvm::BasicBlock *> incoming_blocks = {};

  // TODO: fix me!
  // for (auto incoming_idx = 0; incoming_idx < I.getNumIncomingValues();
  //      incoming_idx++) {
  //   // Get the incoming basic block.
  //   auto *incoming_basic_block = I.getIncomingBlock(incoming_idx);

  //   // TODO: implement me.
  //   auto *incoming_expr = nullptr;

  //   // Append it to the Control PHI.
  //   incoming_expressions.push_back(incoming_expr);
  //   incoming_blocks.push_back(incoming_basic_block);
  // }

  // If there is only one incoming value, just return that expression.
  if (incoming_expressions.size() == 1) {
    return incoming_expressions.at(0);
  }

  // Create the PHIExpression.
  auto *phi_expr = new PHIExpression(incoming_expressions, incoming_blocks);
  MEMOIR_NULL_CHECK(phi_expr, "Could not construct the PHI Expression");

  return phi_expr;
}

ValueExpression *SizeAnalysis::visitRetPHIInst(RetPHIInst &I) {
  // Visit each of the incoming edges.
  vector<ValueExpression *> incoming_expressions = {};
  vector<llvm::ReturnInst *> incoming_returns = {};

  // TODO: updateme!
  // for (auto incoming_idx = 0; incoming_idx < C.getNumIncoming();
  //      incoming_idx++) {
  //   // TODO: implement me!
  //   // Get the incoming basic block.
  //   auto *incoming_return = nullptr;

  //   // TODO: implement me!
  //   // Get the incoming collection.

  //   // TODO: implement me!
  //   // Get the size expression.
  //   auto *incoming_expr = nullptr;

  //   // Append it to the Control PHI.
  //   incoming_expressions.push_back(incoming_expr);
  //   incoming_returns.push_back(incoming_return);
  // }

  // If there is only one incoming value, just return that expression.
  if (incoming_expressions.size() == 1) {
    return incoming_expressions.at(0);
  }

  // Otherwise, we don't have a way to construct interprocedural expressions
  // like this.
  // NOTE: We are normalizing to a single return with noelle-norm so this case
  // shouldn't show up.
  return nullptr;
}

ValueExpression *SizeAnalysis::visitArgPHIInst(ArgPHIInst &I) {
  // TODO: We need to have a way to represent interprocedural ValueExpressions
  // for ArgPHI and RetPHI.
  // NOTE: For now, we will only handle the case where there is one incoming
  // call for the argument.
  // TODO: implement me!
  return nullptr;
}

ValueExpression *SizeAnalysis::visitDefPHIInst(DefPHIInst &I) {
  // Visit the collection being redefined.
  return this->getSize(I.getDefinedCollection());
}

ValueExpression *SizeAnalysis::visitUsePHIInst(UsePHIInst &I) {
  // Visit the collection being used.
  return this->getSize(I.getUsedCollection());
}

} // namespace llvm::memoir
