#include "memoir/analysis/ValueExpression.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/support/InternalDatatypes.hpp"
#include "memoir/support/Print.hpp"

namespace llvm::memoir {

// Internal helper functions.
llvm::Argument *ValueExpression::handleCallContext(
    ValueExpression &Expr,
    llvm::Value &materialized_value,
    llvm::CallBase &call_context,
    MemOIRBuilder *builder,
    const llvm::DominatorTree *DT) {

  println("Handling call context");
  println("  for ", Expr);
  println("  at  ", call_context);

  // Get the called function.
  auto *called_function = call_context.getCalledFunction();
  if (!called_function) {
    return nullptr;
  }

  // Get the module.
  auto *module = called_function->getParent();

  // Get information from the old function type.
  auto &old_func_type = sanitize(called_function->getFunctionType(),
                                 "Couldn't determine the old function type");
  auto *return_type = old_func_type.getReturnType();
  auto param_types = old_func_type.params().vec();
  auto is_var_arg = old_func_type.isVarArg();

  // Add the new argument type.
  auto new_arg_index = param_types.size();
  println(Expr.getLLVMType());
  param_types.push_back(Expr.getLLVMType());

  // Construct the new function type.
  auto *new_func_type = FunctionType::get(return_type, param_types, is_var_arg);

  // Create an empty function to clone into.
  auto *new_function = Function::Create(new_func_type,
                                        called_function->getLinkage(),
                                        called_function->getName(),
                                        module);

  // Version the function.
  auto *vmap = new ValueToValueMapTy();
  llvm::SmallVector<llvm::ReturnInst *, 8> returns;
  llvm::CloneFunctionInto(new_function, called_function, *vmap, false, returns);

  // Remap the function.
  ValueMapper mapper(*vmap);
  for (auto &old_arg : called_function->args()) {
    auto *new_arg = new_function->arg_begin() + old_arg.getArgNo();
    for (auto &BB : *new_function) {
      for (auto &I : BB) {
        I.replaceUsesOfWith(&old_arg, new_arg);
      }
    }
  }

  // Saved the cloned function.
  Expr.function_version = new_function;
  Expr.version_mapping = vmap;

  // Create a new builder if needed.
  bool new_builder = false;
  if (builder == nullptr) {
    builder = new MemOIRBuilder(&call_context);
    new_builder = true;
  }

  // Now, we need to rebuild the call with the new function type and the new
  // argument.

  // Grab the fixed argument list from the old call.
  vector<llvm::Value *> new_arguments = {};
  auto num_fixed_args = old_func_type.getNumParams();
  for (auto arg_idx = 0; arg_idx < num_fixed_args; arg_idx++) {
    auto *fixed_arg = call_context.getArgOperand(arg_idx);
    new_arguments.push_back(fixed_arg);
  }

  // Append the new argument.
  new_arguments.push_back(&materialized_value);

  // Append the variadic arguments, if there are any.
  if (is_var_arg) {
    for (auto var_arg_idx = num_fixed_args;
         var_arg_idx < call_context.getNumOperands();
         var_arg_idx++) {
      auto *var_arg = call_context.getArgOperand(var_arg_idx);
      new_arguments.push_back(var_arg);
    }
  }

  // Rebuild the call with the correct function type.
  auto *versioned_call =
      builder->CreateCall(new_func_type,
                          new_function,
                          llvm::ArrayRef<llvm::Value *>(new_arguments));

  Expr.call_version = versioned_call;

  // Delete the builder if we created one.
  if (new_builder) {
    delete builder;
  }

  // Get the new argument.
  auto *first_argument = new_function->arg_begin();
  auto &materialized_argument = sanitize(first_argument + num_fixed_args,
                                         "Could not get the new argument");

  // Return the new argument.
  return &materialized_argument;
}

// ConstantExpression.
bool ConstantExpression::isAvailable(llvm::Instruction &IP,
                                     const llvm::DominatorTree *DT,
                                     llvm::CallBase *call_context) {
  return true;
}

llvm::Value *ConstantExpression::materialize(llvm::Instruction &IP,
                                             MemOIRBuilder *builder,
                                             const llvm::DominatorTree *DT,
                                             llvm::CallBase *call_context) {
  println("Materializing ", *this);
  return &C;
}

// VariableExpression
bool VariableExpression::isAvailable(llvm::Instruction &IP,
                                     const llvm::DominatorTree *DT,
                                     llvm::CallBase *call_context) {
  return false;
}

llvm::Value *VariableExpression::materialize(llvm::Instruction &IP,
                                             MemOIRBuilder *builder,
                                             const llvm::DominatorTree *DT,
                                             llvm::CallBase *call_context) {
  println("Materializing ", *this);
  return nullptr;
}

// ArgumentExpression
bool ArgumentExpression::isAvailable(llvm::Instruction &IP,
                                     const llvm::DominatorTree *DT,
                                     llvm::CallBase *call_context) {
  // Get the insertion basic block and function.
  auto insertion_bb = IP.getParent();
  if (!insertion_bb) {
    println("Couldn't get the insertion basic block");
    return false;
  }
  auto insertion_func = insertion_bb->getParent();
  if (!insertion_func) {
    println("Couldn't get the insertion function");
    return false;
  }

  // Check that the insertion point and the argument are in the same function.
  auto value_func = this->A.getParent();
  if (value_func == insertion_func) {
    return true;
  }

  // See if the argument is available at the calling context.
  // TODO: extend this to have a partial call stack.
  if (call_context) {
    return this->isAvailable(*call_context, DT);
  }

  // Otherwise, the argument is not available.
  return false;
}

llvm::Value *ArgumentExpression::materialize(llvm::Instruction &IP,
                                             MemOIRBuilder *builder,
                                             const llvm::DominatorTree *DT,
                                             llvm::CallBase *call_context) {
  println("Materializing ", *this);
  println("  ", this->A);
  println("  at ", IP);

  // Check if this Argument is available.
  if (!isAvailable(IP, DT, call_context)) {
    return nullptr;
  }

  // Get the insertion basic block and function.
  auto insertion_bb = IP.getParent();
  if (!insertion_bb) {
    println("Couldn't determine insertion basic block");
    return nullptr;
  }
  auto insertion_func = insertion_bb->getParent();
  if (!insertion_func) {
    println("Couldn't determine insertion function");
    return nullptr;
  }

  // Check that the insertion point and the argument are in the same function.
  auto value_func = this->A.getParent();
  if (value_func == insertion_func) {
    println("argument is available, forwarding along.");
    return &(this->A);
  }

  // See if the argument is available at the calling context.
  // TODO: extend this to have a partial call stack.
  if (call_context) {
    if (this->isAvailable(*call_context, DT)) {
      // Handle the call.
      if (auto *materialized_argument =
              handleCallContext(*this, this->A, *call_context, builder, DT)) {

        println("  Materialized: ", *materialized_argument);
        return materialized_argument;
      }
    }
  }

  // Otherwise, the argument is not available.
  return nullptr;
}

// UnknownExpression
bool UnknownExpression::isAvailable(llvm::Instruction &IP,
                                    const llvm::DominatorTree *DT,
                                    llvm::CallBase *call_context) {
  return false;
}

llvm::Value *UnknownExpression::materialize(llvm::Instruction &IP,
                                            MemOIRBuilder *builder,
                                            const llvm::DominatorTree *DT,
                                            llvm::CallBase *call_context) {
  println("Materializing ", *this);
  return nullptr;
}

// BasicExpression
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
  for (auto arg_idx = 0; arg_idx < this->getNumArguments(); arg_idx++) {
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
  println("Materializing ", *this);
  println("  opcode= ", this->opcode);
  if (this->isAvailable(IP, DT, call_context)) {
    println("  Expression is available!");
  }

  MemOIRBuilder local_builder(&IP);

  // Materialize the operands.
  vector<llvm::Value *> materialized_operands = {};
  for (auto *operand_expr : this->arguments) {
    println("Materializing operand");
    println("  ", *operand_expr);
    println("  at ", IP);
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
    println("  materialized: ", *materialized_binary_op);
    return materialized_binary_op;
  }

  // If this is a unary operator, create it.
  if (llvm::Instruction::isUnaryOp(this->opcode)) {
    auto *materialized_unary_op =
        local_builder.CreateUnOp((llvm::Instruction::UnaryOps)this->opcode,
                                 materialized_operands.at(0));
    println("  materialized: ", *materialized_unary_op);
    return materialized_unary_op;
  }

  println("Couldn't materialize the BasicExpression!");
  println(*this);
  if (this->I) {
    println(*this->I);
  }
  return nullptr;
}

// CastExpression
llvm::Value *CastExpression::materialize(llvm::Instruction &IP,
                                         MemOIRBuilder *builder,
                                         const llvm::DominatorTree *DT,
                                         llvm::CallBase *call_context) {
  println("Materializing ", *this);
  if (this->isAvailable(IP, DT, call_context)) {
    println("  Expression is available!");
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
llvm::Value *ICmpExpression::materialize(llvm::Instruction &IP,
                                         MemOIRBuilder *builder,
                                         const llvm::DominatorTree *DT,
                                         llvm::CallBase *call_context) {
  println("Materializing ", *this);
  if (this->isAvailable(IP, DT, call_context)) {
    println("  Expression is available!");
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
llvm::Value *PHIExpression::materialize(llvm::Instruction &IP,
                                        MemOIRBuilder *builder,
                                        const llvm::DominatorTree *DT,
                                        llvm::CallBase *call_context) {
  println("Materializing ", *this);
  // TODO
  return nullptr;
}

// SelectExpression
llvm::Value *SelectExpression::materialize(llvm::Instruction &IP,
                                           MemOIRBuilder *builder,
                                           const llvm::DominatorTree *DT,
                                           llvm::CallBase *call_context) {
  println("Materializing ", *this);

  // Check that this SelectExpression is available.
  if (!this->isAvailable(IP, DT, call_context)) {
    return nullptr;
  }

  // If we are already a materialized instruction, check if we dominate the
  // insertion point. If we do, return the instruction.
  if ((this->I != nullptr) && (DT != nullptr)) {
    println(*this->I);
    if (DT->dominates(this->I, &IP)) {
      println("instruction dominates insertion point, forwarding along.");
      println(*this->I);
      return this->I;
    }
  }

  // If we have a calling context, see if the value is already in the argument
  // list.
  if ((this->I != nullptr) && (call_context != nullptr)) {
    if (call_context->hasArgument(this->I)) {
      println("instruction is available via the call, forwarding the argument");
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
          sanitize(this->materialize(*call_context, builder, DT),
                   "Could not materialize value at the call site");

      println("materialized ", materialized_value);

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
    println("SelectExpression: Condition expression is NULL!");
    return nullptr;
  }
  auto *true_expr = this->getTrueValue();
  if (!true_expr) {
    println("SelectExpression: True value expression is NULL!");
    return nullptr;
  }
  auto *false_expr = this->getFalseValue();
  if (!false_expr) {
    println("SelectExpression: False value expression is NULL!");
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

  println("Materialized: ", *materialized_select);

  return materialized_select;
}

// CallExpression
llvm::Value *CallExpression::materialize(llvm::Instruction &IP,
                                         MemOIRBuilder *builder,
                                         const llvm::DominatorTree *DT,
                                         llvm::CallBase *call_context) {
  println("Materializing ", *this);
  // TODO
  return nullptr;
}

// CollectionExpression
bool CollectionExpression::isAvailable(llvm::Instruction &IP,
                                       const llvm::DominatorTree *DT,
                                       llvm::CallBase *call_context) {

  return false;
}

llvm::Value *CollectionExpression::materialize(llvm::Instruction &IP,
                                               MemOIRBuilder *builder,
                                               const llvm::DominatorTree *DT,
                                               llvm::CallBase *call_context) {
  println("Materializing ", *this);
  // TODO
  return nullptr;
}

// StructExpression
bool StructExpression::isAvailable(llvm::Instruction &IP,
                                   const llvm::DominatorTree *DT,
                                   llvm::CallBase *call_context) {

  return false;
}

llvm::Value *StructExpression::materialize(llvm::Instruction &IP,
                                           MemOIRBuilder *builder,
                                           const llvm::DominatorTree *DT,
                                           llvm::CallBase *call_context) {
  println("Materializing ", *this);
  // TODO
  return nullptr;
}

//  SizeExpression
bool SizeExpression::isAvailable(llvm::Instruction &IP,
                                 const llvm::DominatorTree *DT,
                                 llvm::CallBase *call_context) {
  println("Materializing ", *this);
  if (!CE) {
    println("Could not find the collection expression being sized");
    return false;
  }

  return CE->isAvailable(IP, DT, call_context);
}

llvm::Value *SizeExpression::materialize(llvm::Instruction &IP,
                                         MemOIRBuilder *builder,
                                         const llvm::DominatorTree *DT,
                                         llvm::CallBase *call_context) {
  println("Materializing ", *this);
  if (!CE) {
    return nullptr;
  }

  bool created_builder = false;
  if (!builder) {
    builder = new MemOIRBuilder(&IP);
    created_builder = true;
  }

  // Materialize the collection.
  auto materialized_collection = CE->materialize(IP, builder, DT, call_context);

  // Forward along any function versioning.
  this->function_version = CE->function_version;
  this->version_mapping = CE->version_mapping;
  this->call_version = CE->call_version;

  // If we couldn't materialize the collection, cleanup and return NULL.
  if (!materialized_collection) {
    if (created_builder) {
      delete builder;
    }
    return nullptr;
  }

  // Materialize the call to size.
  auto materialized_size = builder->CreateSizeInst(materialized_collection);

  // Return the LLVM Value that was materialized.
  return &(materialized_size->getCallInst());
}

} // namespace llvm::memoir
