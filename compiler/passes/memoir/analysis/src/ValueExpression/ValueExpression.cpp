#include "memoir/analysis/ValueExpression.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/support/InternalDatatypes.hpp"
#include "memoir/support/Print.hpp"

namespace llvm::memoir {

// LLVM-Style RTTI
ExpressionKind ValueExpression::getKind() const {
  return this->EK;
};

// Accessors.
llvm::Value *ValueExpression::getValue() const {
  return this->value;
}

llvm::Type *ValueExpression::getLLVMType() const {
  if (!this->value) {
    return this->arguments.at(0)->getLLVMType();
  }
  return this->value->getType();
}

Type *ValueExpression::getMemOIRType() const {
  // TODO
  return nullptr;
}

unsigned ValueExpression::getNumArguments() const {
  return this->arguments.size();
}

ValueExpression *ValueExpression::getArgument(unsigned idx) const {
  MEMOIR_ASSERT((idx < this->getNumArguments()),
                "Index out of range for ValueExpression arguments");
  return this->arguments[idx];
}

void ValueExpression::setArgument(unsigned idx, ValueExpression &expr) {
  this->arguments[idx] = &expr;
}

llvm::Function *ValueExpression::getVersionedFunction() {
  return this->function_version;
};

llvm::ValueToValueMapTy *ValueExpression::getValueMapping() {
  return this->version_mapping;
};

llvm::CallBase *ValueExpression::getVersionedCall() {
  return this->call_version;
};

// Analysis and transformation for context-sensitive materialization.
llvm::Argument *ValueExpression::handleCallContext(
    ValueExpression &Expr,
    llvm::Value &materialized_value,
    llvm::CallBase &call_context,
    MemOIRBuilder *builder,
    const llvm::DominatorTree *DT) {

  infoln("Handling call context");
  infoln("  for ", Expr);
  infoln("  at  ", call_context);

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
  infoln(Expr.getLLVMType());
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

} // namespace llvm::memoir
