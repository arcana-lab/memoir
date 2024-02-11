#include "memoir/analysis/ValueNumbering.hpp"

#include "llvm/Transforms/Utils/BasicBlockUtils.h"

namespace llvm::memoir {

// ValueTable implementation.
ValueExpression *ValueTable::lookup(llvm::Value &V) {
  auto found_value = this->table.find(&V);
  if (found_value != this->table.end()) {
    return found_value->second;
  }

  return nullptr;
}

bool ValueTable::insert(llvm::Value &V, ValueExpression &E) {
  this->table[&V] = &E;
  return true;
}

ValueExpression *ValueTable::lookup(llvm::Use &U) {
  if (auto *lookup_value = this->lookup(*U.get())) {
    return lookup_value;
  }

  return nullptr;
}

bool ValueTable::insert(llvm::Use &U, ValueExpression &E) {
  this->table[U.get()] = &E;
  return true;
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

ValueExpression *ValueNumbering::get(MemOIRInst &I) {
  return ValueNumbering::get(I.getCallInst());
}

ValueExpression *ValueNumbering::lookup(llvm::Value &V) {
  // Lookup the LLVM Value. If we find it, return it and delete the temporary.
  auto found_expr = this->VT.lookup(V);
  if (found_expr != nullptr) {
    return found_expr;
  }

  return nullptr;
}

ValueExpression *ValueNumbering::lookup(llvm::Use &U) {
  // Lookup the LLVM Use. If we find it, return it and delete the temporary.
  auto found_expr = this->VT.lookup(U);
  if (found_expr != nullptr) {
    return found_expr;
  }

  return nullptr;
}

ValueExpression *ValueNumbering::lookup(MemOIRInst &I) {
  return this->lookup(I.getCallInst());
}

void ValueNumbering::insert(llvm::Value &V, ValueExpression *expr) {
  this->VT.insert(V, *expr);
}

void ValueNumbering::insert(llvm::Use &U, ValueExpression *expr) {
  this->VT.insert(U, *expr);
}

void ValueNumbering::insert(MemOIRInst &I, ValueExpression *expr) {
  this->insert(I.getCallInst(), expr);
}

ValueExpression *ValueNumbering::lookupOrInsert(llvm::Value &V,
                                                ValueExpression *expr) {
  // Sanity check.
  MEMOIR_NULL_CHECK(expr, "Expression is NULL!");

  // Lookup the LLVM Value. If we find it, return it and delete the temporary.
  if (auto *found_expr = this->lookup(V)) {
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
  if (auto *found_expr = this->lookup(U)) {
    delete expr;
    return found_expr;
  }

  // Otherwise, insert the temporary expr and return.
  this->VT.insert(U, *expr);
  return expr;
}

// Value visitors.
ValueExpression *ValueNumbering::visitUse(llvm::Use &U) {
  // // Check if this use is a collection.
  // if (Type::value_is_collection_type(*U.get())) {
  //   auto collection = U.get();
  //   if (collection) {
  //     return this->lookupOrInsert(U, new CollectionExpression(*collection));
  //   }
  // }

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
  if (auto *found_expr = this->lookup(I)) {
    return found_expr;
  }
  auto *expr = new BasicExpression(I);
  this->insert(I, expr);

  // Otherwise, let's initialize it.
  expr->arguments.reserve(I.getNumOperands());
  for (auto &operand : I.operands()) {
    auto op_expr = this->visitUse(operand);
    MEMOIR_NULL_CHECK(op_expr,
                      "Couldn't determine operand expression of Instruction!");
    expr->arguments.push_back(op_expr);
  }

  // Return it.
  return expr;
}

ValueExpression *ValueNumbering::visitCastInst(llvm::CastInst &I) {
  // Get or create the ValueExpression.
  if (auto *found_expr = this->lookup(I)) {
    return found_expr;
  }
  auto *expr = new CastExpression(I);
  this->insert(I, expr);

  // If it is already initialized correctly, return.
  if (expr->getNumArguments() == I.getNumOperands()) {
    return expr;
  }

  // Otherwise, let's initialize it.
  expr->arguments.reserve(I.getNumOperands());
  for (auto &operand : I.operands()) {
    auto op_expr = this->visitUse(operand);
    MEMOIR_NULL_CHECK(op_expr,
                      "Couldn't determine operand expression of Instruction!");
    expr->arguments.push_back(op_expr);
  }

  // Return it.
  return expr;
}

ValueExpression *ValueNumbering::visitICmpInst(llvm::ICmpInst &I) {
  // Get or create the ValueExpression.
  ValueExpression *expr;
  if (expr = this->lookup(I)) {
    return expr;
  }
  expr = new ICmpExpression(I);
  this->insert(I, expr);

  // If it is already initialized correctly, return.
  if (expr->getNumArguments() == I.getNumOperands()) {
    return expr;
  }

  // Otherwise, let's initialize it.
  expr->arguments.reserve(I.getNumOperands());
  for (auto &operand : I.operands()) {
    auto op_expr = this->visitUse(operand);
    MEMOIR_NULL_CHECK(op_expr,
                      "Couldn't determine operand expression of Instruction!");
    expr->arguments.push_back(op_expr);
  }

  // Return it.
  return expr;
}

ValueExpression *ValueNumbering::visitPHINode(llvm::PHINode &I) {
  // Check if this PHINode can be summarized as a select.
  // TODO: may want to sanity check that this PHI has only two incoming edges.
  auto *phi_bb = I.getParent();
  MEMOIR_NULL_CHECK(phi_bb, "PHI node does not belong to a basic block!");
  llvm::BasicBlock *if_bb = nullptr;
  llvm::BasicBlock *else_bb = nullptr;
  auto *condition_value = llvm::GetIfCondition(phi_bb, if_bb, else_bb);
  if (condition_value) {
    // Construct a SelectExpression for the PHI node.
    if (auto *found_expr = this->lookup(I)) {
      return found_expr;
    }
    auto *expr = new SelectExpression(I);
    this->insert(I, expr);

    // Get the expression of the condition value.
    auto *condition_expr = this->visitValue(*condition_value);
    MEMOIR_NULL_CHECK(condition_expr,
                      "Could not determine the condition expression of select");

    // Figure out which index is which incoming edge.
    auto *first_bb = I.getIncomingBlock(0);
    auto *second_bb = I.getIncomingBlock(1);
    auto true_index = (if_bb == first_bb) ? 0 : 1;
    auto false_index = (else_bb == second_bb) ? 1 : 0;

    // Get the true value.
    ValueExpression *true_expr;
    auto &true_use = I.getOperandUse(true_index);
    if (true_use.get() == &I) {
      true_expr = expr;
    } else {
      true_expr = this->visitUse(true_use);
      MEMOIR_NULL_CHECK(true_expr,
                        "Could not determine true expression of select");
    }

    // Get the false value.
    ValueExpression *false_expr;
    auto &false_use = I.getOperandUse(false_index);
    if (false_use.get() == &I) {
      false_expr = expr;
    } else {
      false_expr = this->visitUse(false_use);
      MEMOIR_NULL_CHECK(false_expr,
                        "Could not determine false expression of select");
    }

    expr->arguments.reserve(3);
    expr->arguments.push_back(condition_expr);
    expr->arguments.push_back(true_expr);
    expr->arguments.push_back(false_expr);

    return expr;
  }

  // Get or create the PHIExpression.
  if (auto *found_expr = this->lookup(I)) {
    return found_expr;
  }
  auto *expr = new PHIExpression(I);
  this->insert(I, expr);

  // Otherwise, let's initialize it.
  expr->arguments.reserve(I.getNumIncomingValues());
  for (auto &operand : I.incoming_values()) {
    // If the operand is the PHI itself, append the newly created expression.
    if (operand.get() == &I) {
      expr->arguments.push_back(expr);
      continue;
    }

    // Otherwise, we will examine the operand to determine its ValueExpression.
    auto *op_expr = this->visitUse(operand);
    MEMOIR_NULL_CHECK(op_expr,
                      "Couldn't determine operand expression of Instruction!");
    expr->arguments.push_back(op_expr);
  }

  // Return it.
  return expr;
}

ValueExpression *ValueNumbering::visitLLVMCallInst(llvm::CallInst &I) {
  if (auto *found_expr = this->lookup(I)) {
    return found_expr;
  }
  auto *expr = new CallExpression(I);
  this->insert(I, expr);

  // TODO: initialize arguments.
  return expr;
}

ValueExpression *ValueNumbering::visitSizeInst(SizeInst &I) {
  // Get the size expression.
  if (auto *found_expr = this->lookup(I)) {
    return found_expr;
  }
  auto *size_expr = new SizeExpression();
  this->insert(I, size_expr);

  // Get the collection expression.
  auto collection_expr =
      this->lookupOrInsert(I.getCollectionAsUse(),
                           this->visitValue(I.getCollection()));

  // Initialize the size expression.
  size_expr->CE = collection_expr;

  // Return.
  return size_expr;
}

} // namespace llvm::memoir
