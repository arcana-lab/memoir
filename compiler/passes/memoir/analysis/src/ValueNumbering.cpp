#include "memoir/analysis/ValueNumbering.hpp"

namespace llvm::memoir {

// ValueTable implementation.
ValueExpression *ValueTable::lookup(llvm::Value &V) {
  // TODO
  return nullptr;
}

bool ValueTable::insert(llvm::Value &V, ValueExpression &E) {
  // TODO
  return false;
}

ValueExpression *ValueTable::lookup(llvm::Use &U) {
  // TODO
  return nullptr;
}

bool ValueTable::insert(llvm::Use &U, ValueExpression &E) {
  // TODO
  return false;
}

// ValueNumbering implementation.
ValueExpression *ValueNumbering::get(llvm::Value &V) {
  // Lookup the LLVM Value. If we find it, return it and delete the temporary.
  auto found_expr = this->VT.lookup(V);
  if (found_expr != nullptr) {
    return found_expr;
  }

  return this->visitValue(V);
}

ValueExpression *ValueNumbering::get(llvm::Use &U) {
  // Lookup the LLVM Use. If we find it, return it.
  auto found_expr = this->VT.lookup(U);
  if (found_expr != nullptr) {
    return found_expr;
  }

  return this->visitUse(U);
}

ValueExpression *ValueNumbering::lookupOrInsert(llvm::Value &V,
                                                ValueExpression *expr) {
  // Sanity check.
  MEMOIR_NULL_CHECK(expr, "Expression is NULL!");

  // Lookup the LLVM Value. If we find it, return it and delete the temporary.
  auto found_expr = this->VT.lookup(V);
  if (found_expr != nullptr) {
    delete expr;
    return found_expr;
  }

  // Otherwise, insert the temporary expr and return.
  this->VT.insert(V, *expr);
  return expr;
}

ValueExpression *ValueNumbering::lookupOrInsert(llvm::Use &U,
                                                ValueExpression *expr) {
  // Sanity check.
  MEMOIR_NULL_CHECK(expr, "Expression is NULL!");

  // Lookup the LLVM Value. If we find it, return it and delete the temporary.
  auto found_expr = this->VT.lookup(U);
  if (found_expr != nullptr) {
    delete expr;
    return found_expr;
  }

  // Otherwise, insert the temporary expr and return.
  this->VT.insert(U, *expr);
  return expr;
}

// Value visitors.
ValueExpression *ValueNumbering::visitUse(llvm::Use &U) {
  // Check if this use is a collection.
  auto collection = CollectionAnalysis::analyze(U);
  if (collection) {
    return this->lookupOrInsert(U, new CollectionExpression(*collection));
  }

  // Otherwise, visit the Value being used.
  auto used_value = U.get();
  if (used_value) {
    return this->visitValue(*used_value);
  }

  // If there's no value being used, return NULL.
  return nullptr;
}

ValueExpression *ValueNumbering::visitValue(llvm::Value &V) {
  if (auto arg = dyn_cast<llvm::Argument>(&V)) {
    return this->visitArgument(*arg);
  } else if (auto inst = dyn_cast<llvm::Instruction>(&V)) {
    return this->visit(*inst);
  } else if (auto constant = dyn_cast<llvm::Constant>(&V)) {
    return this->visitConstant(*constant);
  }

  return this->lookupOrInsert(V, new UnknownExpression());
}

ValueExpression *ValueNumbering::visitArgument(llvm::Argument &A) {
  return this->lookupOrInsert(A, new ArgumentExpression(A));
}

ValueExpression *ValueNumbering::visitConstant(llvm::Constant &C) {
  return this->lookupOrInsert(C, new ConstantExpression(C));
}

// InstVisitor implementation.

ValueExpression *ValueNumbering::visitInstruction(llvm::Instruction &I) {

  // Get or create the ValueExpression.
  auto expr = this->lookupOrInsert(I, new BasicExpression(I));

  // If it is already initialized correctly, return.
  if (expr->getNumArguments() == I.getNumOperands()) {
    return expr;
  }

  // Otherwise, let's initialize it.
  expr->arguments.assign(I.getNumOperands(), nullptr);
  for (auto &operand : I.operands()) {
    auto op_expr = this->visitUse(operand);
    MEMOIR_NULL_CHECK(op_expr,
                      "Couldn't determine operand expression of Instruction!");
    expr->setArgument(operand.getOperandNo(), *op_expr);
  }

  // Return it.
  return expr;
}

ValueExpression *ValueNumbering::visitCastInst(llvm::CastInst &I) {

  // Get or create the ValueExpression.
  auto expr = this->lookupOrInsert(I, new CastExpression(I));

  // If it is already initialized correctly, return.
  if (expr->getNumArguments() == I.getNumOperands()) {
    return expr;
  }

  // Otherwise, let's initialize it.
  expr->arguments.assign(I.getNumOperands(), nullptr);
  for (auto &operand : I.operands()) {
    auto op_expr = this->visitUse(operand);
    MEMOIR_NULL_CHECK(op_expr,
                      "Couldn't determine operand expression of Instruction!");
    expr->setArgument(operand.getOperandNo(), *op_expr);
  }

  // Return it.
  return expr;
}

ValueExpression *ValueNumbering::visitICmpInst(llvm::ICmpInst &I) {

  // Get or create the ValueExpression.
  auto expr = this->lookupOrInsert(I, new ICmpExpression(I));

  // If it is already initialized correctly, return.
  if (expr->getNumArguments() == I.getNumOperands()) {
    return expr;
  }

  // Otherwise, let's initialize it.
  expr->arguments.assign(I.getNumOperands(), nullptr);
  for (auto &operand : I.operands()) {
    auto op_expr = this->visitUse(operand);
    MEMOIR_NULL_CHECK(op_expr,
                      "Couldn't determine operand expression of Instruction!");
    expr->setArgument(operand.getOperandNo(), *op_expr);
  }

  // Return it.
  return expr;
}

ValueExpression *ValueNumbering::visitPHINode(llvm::PHINode &I) {
  // Check if this PHINode can be summarized as a select.
  // TODO: may want to sanity check that this PHI has only two incoming edges.
  auto &phi_bb =
      sanitize(I.getParent(), "PHI node does not belong to a basic block!");
  llvm::BasicBlock *if_bb;
  llvm::BasicBlock *else_bb;
  auto *condition_value = llvm::GetIfCondition(&phi_bb, if_bb, else_bb);
  if (condition_value) {
    // Get the expression of the condition value.
    auto &condition_expr =
        sanitize(this->visitValue(*condition_value),
                 "Could not construct an expression for the condition");
    // Figure out which index is which incoming edge.
    auto *first_bb = I.getIncomingBlock(0);
    auto *second_bb = I.getIncomingBlock(1);
    auto true_index = (if_bb == first_bb) ? 0 : 1;
    auto false_index = (else_bb == second_bb) ? 1 : 0;

    // Get the true value.
    auto &true_use = I.getOperandUse(true_index);
    auto &true_expr =
        sanitize(this->visitUse(true_use),
                 "Could not determine the expression for the true value");

    // Get the false value.
    auto &false_use = I.getOperandUse(false_index);
    auto &false_expr =
        sanitize(this->visitUse(false_use),
                 "Could not determine the expression for the false value");

    // Construct a SelectExpression for the PHI node.
    auto *expr = this->lookupOrInsert(
        I,
        new SelectExpression(condition_expr, true_expr, false_expr));

    return expr;
  }

  // Get or create the PHIExpression.
  auto *expr = this->lookupOrInsert(I, new PHIExpression(I));

  // If it is already initialized correctly, return.
  if (expr->getNumArguments() == I.getNumIncomingValues()) {
    return expr;
  }

  // Otherwise, let's initialize it.
  expr->arguments.assign(I.getNumIncomingValues(), nullptr);
  for (auto &operand : I.incoming_values()) {
    auto *op_expr = this->visitUse(operand);
    MEMOIR_NULL_CHECK(op_expr,
                      "Couldn't determine operand expression of Instruction!");
    expr->setArgument(operand.getOperandNo(), *op_expr);
  }

  // Return it.
  return expr;
}

ValueExpression *ValueNumbering::visitLLVMCallInst(llvm::CallInst &I) {
  // TODO
  return this->lookupOrInsert(I, new CallExpression(I));
}

ValueExpression *ValueNumbering::visitSizeInst(SizeInst &I) {
  // Get the size expression.
  auto expr = this->lookupOrInsert(I.getCallInst(), new SizeExpression());
  auto size_expr = dyn_cast<SizeExpression>(expr);
  MEMOIR_NULL_CHECK(size_expr, "SizeInst is not a SizeExpression!");

  // If size expression is already initialized, return.
  if (size_expr->CE != nullptr) {
    return size_expr;
  }

  // Get the collection expression.
  auto &C = I.getCollection();
  auto op_expr = this->lookupOrInsert(I.getCollectionOperandAsUse(),
                                      new CollectionExpression(C));
  auto collection_expr = dyn_cast<CollectionExpression>(op_expr);
  MEMOIR_NULL_CHECK(
      collection_expr,
      "SizeInst does not an operand of type CollectionExpression!");

  // Initialize the size expression.
  size_expr->CE = collection_expr;

  // Return.
  return size_expr;
}

} // namespace llvm::memoir
