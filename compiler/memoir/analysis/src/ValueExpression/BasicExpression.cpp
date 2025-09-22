#include "memoir/analysis/ValueExpression.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/support/DataTypes.hpp"
#include "memoir/support/Print.hpp"

/*
 * This file contains implementations for LLVM operator expressions and their
 * specializations.
 *
 * Author(s): Tommy McMichen
 * Created: May 25, 2023
 */

namespace memoir {

bool BasicExpression::isAvailable(llvm::Instruction &IP,
                                  const llvm::DominatorTree *DT,
                                  llvm::CallBase *call_context) {
  // Get the insertion basic block and function.
  auto insertion_bb = IP.getParent();
  if (!insertion_bb) {
    return false;
  }
  auto insertion_func = insertion_bb->getParent();
  if (!insertion_func) {
    return false;
  }

  // If this Instruction exists, check if it dominates the insertion point.
  if ((this->I != nullptr) && (DT != nullptr)) {
    if (DT->dominates(this->I, &IP)) {
      return true;
    }

    // If we have a calling context, check if this Instruction dominates the
    // call.
    if (call_context != nullptr) {
      if (DT->dominates(this->I, call_context)) {
        return true;
      }
    }
  }

  // If this Instruction exists, check that the instruction has no side
  // effects.
  if (this->I != nullptr) {
    if (this->I->mayHaveSideEffects()) {
      return false;
    }
  }

  // Otherwise, iterate on the arguments of this expression.
  bool is_available = true;
  for (unsigned arg_idx = 0; arg_idx < this->getNumArguments(); arg_idx++) {
    auto *arg_expr = this->getArgument(arg_idx);
    MEMOIR_NULL_CHECK(arg_expr,
                      "BasicExpression has a NULL expression as an argument");
    is_available &= arg_expr->isAvailable(IP, DT, call_context);
  }

  return is_available;
}

llvm::Value *BasicExpression::materialize(llvm::Instruction &IP,
                                          MemOIRBuilder *builder,
                                          const llvm::DominatorTree *DT,
                                          llvm::CallBase *call_context) {
  debugln("Materializing ", *this);
  debugln("  opcode= ", this->opcode);
  if (this->isAvailable(IP, DT, call_context)) {
    debugln("  Expression is available!");
  }

  MemOIRBuilder local_builder(&IP);

  // Materialize the operands.
  Vector<llvm::Value *> materialized_operands = {};
  for (auto *operand_expr : this->arguments) {
    debugln("Materializing operand");
    debugln("  ", *operand_expr);
    debugln("  at ", IP);
    auto *materialized_operand =
        operand_expr->materialize(IP, nullptr, DT, call_context);
    MEMOIR_NULL_CHECK(materialized_operand,
                      "Could not materialize the operand");
    materialized_operands.push_back(materialized_operand);
  }

  // If this is a binary operator, create it.
  if (llvm::Instruction::isBinaryOp(this->opcode)) {
    auto *materialized_binary_op =
        local_builder.CreateBinOp((llvm::Instruction::BinaryOps)this->opcode,
                                  materialized_operands.at(0),
                                  materialized_operands.at(1));
    debugln("  materialized: ", *materialized_binary_op);
    return materialized_binary_op;
  }

  // If this is a unary operator, create it.
  if (llvm::Instruction::isUnaryOp(this->opcode)) {
    auto *materialized_unary_op =
        local_builder.CreateUnOp((llvm::Instruction::UnaryOps)this->opcode,
                                 materialized_operands.at(0));
    debugln("  materialized: ", *materialized_unary_op);
    return materialized_unary_op;
  }

  debugln("Couldn't materialize the BasicExpression!");
  debugln(*this);
  if (this->I) {
    debugln(*this->I);
  }
  return nullptr;
}

// CastExpression
ValueExpression &CastExpression::getOperand() const {
  return *(this->arguments.at(0));
};

llvm::Type &CastExpression::getDestType() const {
  return this->dest_type;
};

llvm::Value *CastExpression::materialize(llvm::Instruction &IP,
                                         MemOIRBuilder *builder,
                                         const llvm::DominatorTree *DT,
                                         llvm::CallBase *call_context) {
  debugln("Materializing ", *this);
  if (this->isAvailable(IP, DT, call_context)) {
    debugln("  Expression is available!");
  }

  MemOIRBuilder local_builder(&IP);

  // Materialize the LHS.
  auto *materialized_operand =
      this->getOperand().materialize(IP, nullptr, DT, call_context);
  MEMOIR_NULL_CHECK(materialized_operand,
                    "Could not materialize the operand to be casted");

  // Materialize the ICmpInst.
  auto *materialized_cast =
      local_builder.CreateCast((llvm::Instruction::CastOps)this->opcode,
                               materialized_operand,
                               &this->getDestType());

  return materialized_cast;
}

// ICmpExpression
llvm::CmpInst::Predicate ICmpExpression::getPredicate() const {
  return this->predicate;
}

ValueExpression *ICmpExpression::getLHS() const {
  return this->arguments.at(0);
}

ValueExpression *ICmpExpression::getRHS() const {
  return this->arguments.at(1);
}

llvm::Value *ICmpExpression::materialize(llvm::Instruction &IP,
                                         MemOIRBuilder *builder,
                                         const llvm::DominatorTree *DT,
                                         llvm::CallBase *call_context) {
  debugln("Materializing ", *this);
  if (this->isAvailable(IP, DT, call_context)) {
    debugln("  Expression is available!");
  }

  MemOIRBuilder local_builder(&IP);

  // Materialize the LHS.
  auto *materialized_lhs =
      this->getLHS()->materialize(IP, nullptr, DT, call_context);
  MEMOIR_NULL_CHECK(materialized_lhs, "Could not materialize the LHS");

  // Materialize the RHS.
  auto *materialized_rhs =
      this->getRHS()->materialize(IP, nullptr, DT, call_context);
  MEMOIR_NULL_CHECK(materialized_rhs, "Could not materialize the RHS");

  // Materialize the ICmpInst.
  auto *materialized_icmp = local_builder.CreateICmp(this->getPredicate(),
                                                     materialized_lhs,
                                                     materialized_rhs);

  return materialized_icmp;
}

// PHIExpression
llvm::BasicBlock &PHIExpression::getIncomingBlock(unsigned index) const {
  if (this->phi != nullptr) {
    return MEMOIR_SANITIZE(this->phi->getIncomingBlock(index),
                           "Couldn't get the incoming basic block");
  }

  // Otherwise, get it from the vector.
  MEMOIR_ASSERT(
      (index < this->incoming_blocks.size()),
      "Index out of range of incoming basic blocks for PHIExpression");
  return *(this->incoming_blocks.at(index));
}

llvm::Value *PHIExpression::materialize(llvm::Instruction &IP,
                                        MemOIRBuilder *builder,
                                        const llvm::DominatorTree *DT,
                                        llvm::CallBase *call_context) {
  debugln("Materializing ", *this);
  // TODO
  return nullptr;
}

// SelectExpression
ValueExpression *SelectExpression::getCondition() const {
  return this->getArgument(0);
}

ValueExpression *SelectExpression::getTrueValue() const {
  return this->getArgument(1);
}

ValueExpression *SelectExpression::getFalseValue() const {
  return this->getArgument(2);
}

llvm::Type *SelectExpression::getLLVMType() const {
  return this->getTrueValue()->getLLVMType();
}

llvm::PHINode *SelectExpression::getPHI() const {
  return this->phi;
}

llvm::Value *SelectExpression::materialize(llvm::Instruction &IP,
                                           MemOIRBuilder *builder,
                                           const llvm::DominatorTree *DT,
                                           llvm::CallBase *call_context) {
  debugln("Materializing ", *this);

  // Check that this SelectExpression is available.
  if (!this->isAvailable(IP, DT, call_context)) {
    return nullptr;
  }

  // If we are already a materialized instruction, check if we dominate the
  // insertion point. If we do, return the instruction.
  if ((this->I != nullptr) && (DT != nullptr)) {
    debugln(*this->I);
    if (DT->dominates(this->I, &IP)) {
      debugln("instruction dominates insertion point, forwarding along.");
      debugln(*this->I);
      return this->I;
    }
  }

  // If we have a calling context, see if the value is already in the argument
  // list.
  if ((this->I != nullptr) && (call_context != nullptr)) {
    if (call_context->hasArgument(this->I)) {
      debugln("instruction is available via the call, forwarding the argument");
      // FIXME: return the argument instead of the instruction.
      return this->I;
    }
  }

  // Handle the calling context.
  // See if the argument is available at the calling context.
  // TODO: extend this to have a partial call stack.
  if (call_context != nullptr) {
    if (this->isAvailable(*call_context, DT)) {
      // Materialize the value.
      auto &materialized_value =
          MEMOIR_SANITIZE(this->materialize(*call_context, builder, DT),
                          "Could not materialize value at the call site");

      debugln("materialized ", materialized_value);

      // Handle the call.
      if (auto *materialized_argument = handleCallContext(*this,
                                                          materialized_value,
                                                          *call_context,
                                                          builder,
                                                          DT)) {
        return materialized_argument;
      }
    }
  }

  // Otherwise, let's materialize the operands and build a new select.
  // Sanity check the operands.
  auto *condition_expr = this->getCondition();
  if (!condition_expr) {
    debugln("SelectExpression: Condition expression is NULL!");
    return nullptr;
  }
  auto *true_expr = this->getTrueValue();
  if (!true_expr) {
    debugln("SelectExpression: True value expression is NULL!");
    return nullptr;
  }
  auto *false_expr = this->getFalseValue();
  if (!false_expr) {
    debugln("SelectExpression: False value expression is NULL!");
    return nullptr;
  }

  // Create a builder, if it doesn't exist.
  bool created_builder = false;
  if (!builder) {
    builder = new MemOIRBuilder(&IP);
    created_builder = true;
  }

  // Materialize the operands.
  auto *materialized_condition =
      condition_expr->materialize(IP, builder, DT, call_context);
  if (!materialized_condition) {
    if (created_builder) {
      delete builder;
    }
    return nullptr;
  }

  if (condition_expr->function_version) {
    // Forward along any function versioning.
    this->function_version = condition_expr->function_version;
    this->version_mapping = condition_expr->version_mapping;
    this->call_version = condition_expr->call_version;

    call_context = condition_expr->call_version;
  }

  auto *materialized_true =
      true_expr->materialize(IP, builder, DT, call_context);
  if (!materialized_true) {
    if (created_builder) {
      delete builder;
    }
    return nullptr;
  }

  if (true_expr->function_version) {
    // Forward along any function versioning.
    this->function_version = true_expr->function_version;
    this->version_mapping = true_expr->version_mapping;
    this->call_version = true_expr->call_version;

    call_context = true_expr->call_version;
  }

  auto *materialized_false =
      false_expr->materialize(IP, builder, DT, call_context);
  if (!materialized_false) {
    if (created_builder) {
      delete builder;
    }
    return nullptr;
  }

  // Materialize the select instruction.
  auto materialized_select = builder->CreateSelect(materialized_condition,
                                                   materialized_true,
                                                   materialized_false);
  MEMOIR_NULL_CHECK(
      materialized_select,
      "SelectExpression: Could not create the materialized select.");

  debugln("Materialized: ", *materialized_select);

  return materialized_select;
}

// CallExpression
llvm::Value *CallExpression::materialize(llvm::Instruction &IP,
                                         MemOIRBuilder *builder,
                                         const llvm::DominatorTree *DT,
                                         llvm::CallBase *call_context) {
  debugln("Materializing ", *this);
  // TODO
  return nullptr;
}

} // namespace memoir
