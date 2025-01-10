#include "SSAConstructionVisitor.hpp"

#include "llvm/IR/User.h"

#include "memoir/support/Assert.hpp"
#include "memoir/support/Casting.hpp"
#include "memoir/support/Print.hpp"

#include "memoir/utility/Metadata.hpp"

namespace llvm::memoir {

#define TYPE_ERROR(I)                                                          \
  {                                                                            \
    println("Type error:\n  ", I);                                             \
    MEMOIR_UNREACHABLE("Invalid type!");                                       \
  }

llvm::Value *SSAConstructionVisitor::update_reaching_definition(
    llvm::Value *variable,
    MemOIRInst &I) {
  return this->update_reaching_definition(variable, I.getCallInst());
}

llvm::Value *SSAConstructionVisitor::update_reaching_definition(
    llvm::Value *variable,
    llvm::Instruction &I) {
  return this->update_reaching_definition(variable, &I);
}

llvm::Value *SSAConstructionVisitor::update_reaching_definition(
    llvm::Value *variable,
    llvm::Instruction *program_point) {

  // Search through the chain of definitions for variable until we find the
  // closest definition that dominates the program point. Then update the
  // reaching definition.
  auto *reaching_variable = variable;

  debugln("Computing reaching definition:");
  debugln("  for", *variable);
  debugln("  at ", *program_point);

  do {
    auto found_reaching_definition =
        this->reaching_definitions.find(reaching_variable);
    if (found_reaching_definition == this->reaching_definitions.end()) {
      reaching_variable = nullptr;
      break;
    }

    auto *next_reaching_variable = found_reaching_definition->second;

    if (next_reaching_variable) {
      debugln("=> ", *next_reaching_variable);
    } else {
      debugln("=> NULL");
    }

    reaching_variable = next_reaching_variable;

    // If the reaching definition dominates the program point, update it.
    // If this is an instruction, consult the dominator tree.
    if (auto *reaching_definition =
            dyn_cast_or_null<llvm::Instruction>(reaching_variable)) {
      if (this->DT.dominates(reaching_definition, program_point)) {
        break;
      }
    }

    // Otherwise, if this is an argument; it dominates all program points in the
    // function.
    else if (auto *reaching_definition_as_argument =
                 dyn_cast_or_null<llvm::Argument>(reaching_variable)) {
      break;
    }

  } while (reaching_variable != nullptr && reaching_variable != variable);

  if (reaching_variable != nullptr) {
    this->reaching_definitions[variable] = reaching_variable;
  }

  return reaching_variable;
}

void SSAConstructionVisitor::set_reaching_definition(
    llvm::Value *variable,
    llvm::Value *reaching_definition) {
  this->reaching_definitions[variable] = reaching_definition;
}

void SSAConstructionVisitor::set_reaching_definition(
    llvm::Value *variable,
    MemOIRInst *reaching_definition) {
  this->set_reaching_definition(variable, &reaching_definition->getCallInst());
}

void SSAConstructionVisitor::set_reaching_definition(
    MemOIRInst *variable,
    llvm::Value *reaching_definition) {
  this->set_reaching_definition(&variable->getCallInst(), reaching_definition);
}

void SSAConstructionVisitor::set_reaching_definition(
    MemOIRInst *variable,
    MemOIRInst *reaching_definition) {
  this->set_reaching_definition(&variable->getCallInst(),
                                &reaching_definition->getCallInst());
}

void SSAConstructionVisitor::mark_for_cleanup(llvm::Instruction &I) {
  this->instructions_to_delete.insert(&I);
}

void SSAConstructionVisitor::mark_for_cleanup(MemOIRInst &I) {
  this->mark_for_cleanup(I.getCallInst());
}

void SSAConstructionVisitor::prepare_keywords(MemOIRBuilder &builder,
                                              MemOIRInst &I,
                                              Type *element_type) {
  // Prepare each keyword.
  for (auto kw : I.keywords()) {
    if (auto value_kw = try_cast<ValueKeyword>(kw)) {
      // Coerce the value to the correct type.
      MEMOIR_ASSERT(element_type,
                    "Cannot prepare ValueKeyword with no element type");
      MEMOIR_ASSERT(
          Type::is_primitive_type(*element_type),
          "Invalid use of 'value' keyword with non-primitive element type");
      auto *llvm_type = element_type->get_llvm_type(builder.getContext());
      auto *coerced =
          builder.CreateTrunc(&value_kw->getValue(), llvm_type, "coerce.");

      value_kw->getValueAsUse().set(coerced);

    } else if (auto input_kw = try_cast<InputKeyword>(kw)) {
      // Update the reaching definition of the input collection.
      auto *input_orig = &input_kw->getInput();
      auto *input_curr = update_reaching_definition(input_orig, I);

      input_kw->getInputAsUse().set(input_curr);
    }
  }

  return;
}

SSAConstructionVisitor::SSAConstructionVisitor(
    llvm::DominatorTree &DT,
    ordered_set<llvm::Value *> memoir_names,
    map<llvm::PHINode *, llvm::Value *> inserted_phis,
    SSAConstructionStats *stats,
    bool construct_use_phis)
  : DT(DT),
    inserted_phis(inserted_phis),
    construct_use_phis(construct_use_phis),
    stats(stats) {
  this->reaching_definitions = {};
  for (auto *name : memoir_names) {
    this->set_reaching_definition(name, name);
  }
}

void SSAConstructionVisitor::visitInstruction(llvm::Instruction &I) {

  for (auto &operand_use : I.operands()) {
    auto *operand_value = operand_use.get();
    if (not Type::value_is_object(*operand_value)) {
      continue;
    }

    auto *reaching_operand = update_reaching_definition(operand_value, I);
    operand_use.set(reaching_operand);
  }

  if (Type::value_is_object(I)) {
    this->set_reaching_definition(&I, &I);
  }

  return;
}

void SSAConstructionVisitor::visitPHINode(llvm::PHINode &I) {
  auto found_inserted_phi = this->inserted_phis.find(&I);
  if (found_inserted_phi != this->inserted_phis.end()) {
    // If this was a PHI inserted by us, update the named variable's reaching
    // definition.
    auto *named_variable = found_inserted_phi->second;
    auto *reaching_definition =
        this->update_reaching_definition(named_variable, I);

    // Set the reaching definition for the named variable.
    this->set_reaching_definition(named_variable, &I);
    if (reaching_definition == &I) {
      this->set_reaching_definition(&I, reaching_definition);
    } else {
      this->set_reaching_definition(&I, reaching_definition);
    }
  } else {
    this->set_reaching_definition(&I, &I);
  }

  return;
}

void SSAConstructionVisitor::visitLLVMCallInst(llvm::CallInst &I) {
  for (auto &arg_use : I.data_ops()) {
    auto *arg_value = arg_use.get();
    if (not Type::value_is_object(*arg_value)) {
      continue;
    }

    // Update the use to use the current reaching definition.
    auto *reaching = update_reaching_definition(arg_value, I);
    arg_use.set(reaching);

    // Build a RetPHI for the call.
    MemOIRBuilder builder(&I, true);
    auto *ret_phi = builder.CreateRetPHI(reaching, I.getCalledOperand());
    auto *ret_phi_value = &ret_phi->getResultCollection();

    // Update the reaching definitions.
    this->set_reaching_definition(arg_value, ret_phi_value);
    this->set_reaching_definition(ret_phi_value, reaching);
  }

  // Create a new reaching definition for the returned value, if it's a MEMOIR
  // collection.
  if (Type::value_is_object(I)) {
    this->set_reaching_definition(&I, &I);
  }

  return;
}

void SSAConstructionVisitor::visitReturnInst(llvm::ReturnInst &I) {

  // If the returned value is a collection:
  auto *return_value = I.getReturnValue();
  if (return_value != nullptr and Type::value_is_object(*return_value)) {

    // Update the reaching definition of the return value.
    auto *return_reaching = update_reaching_definition(return_value, I);
    I.setOperand(0, return_reaching);
  }

  // Fetch the parent function.
  auto *function = I.getFunction();

  // If it is NULL, skip the instruction.
  if (function == nullptr) {
    return;
  }

  // For each collection argument passed into the function, record its current
  // reaching definition.
  for (auto &A : function->args()) {

    // Skip non-collection arguments.
    if (not Type::value_is_object(A)) {
      continue;
    }

    // Get the reaching definition.
    auto *reaching = update_reaching_definition(&A, I);

    // If the reaching definition is the returned value, skip it.
    if (I.getReturnValue() == reaching) {
      continue;
    }

    // Get the definining instruction of the live out.
    auto *live_out_def = dyn_cast<llvm::Instruction>(reaching);
    if (not live_out_def) {
      continue;
    }

    // Attach a LiveOutMetadata to the live-out instruction.
    auto live_out_metadata =
        Metadata::get_or_add<LiveOutMetadata>(*live_out_def);
    live_out_metadata.setArgNo(A.getArgNo());
  }

  return;
}

void SSAConstructionVisitor::visitFoldInst(FoldInst &I) {

  // Update the reaching definition for the collection operand.
  auto &collection_use = I.getObjectAsUse();
  auto *reaching_collection =
      update_reaching_definition(collection_use.get(), I);
  collection_use.set(reaching_collection);

  // Update the reaching definitions for the initial value, if it is a
  // collection.
  auto &initial = I.getInitial();
  if (Type::value_is_object(initial)) {
    auto *reaching = update_reaching_definition(&initial, I);
    I.getInitialAsUse().set(reaching);
  }

  // For each of the closed collections:
  if (auto closed_keyword = I.get_keyword<ClosedKeyword>()) {
    for (auto &closed_use : closed_keyword->arg_operands()) {
      auto *closed = closed_use.get();

      // If the closed value is not a collection type, skip it.
      if (not Type::value_is_object(*closed)) {
        continue;
      }

      // Update the use to use the current reaching definition.
      auto *reaching = update_reaching_definition(closed, I);
      closed_use.set(reaching);

      // Inspec the called function, and see if there are any live-out values.

      // Insert a RetPHI for the fold operation.
      MemOIRBuilder builder(I, true);
      auto *function = I.getCallInst().getCalledFunction();
      auto *ret_phi = builder.CreateRetPHI(reaching, function);
      auto *ret_phi_value = &ret_phi->getResultCollection();

      // Update the reaching definitions.
      this->set_reaching_definition(closed, ret_phi_value);
      this->set_reaching_definition(ret_phi_value, reaching);
    }
  }

  // Create a new reaching definition for the returned value, if it's a MEMOIR
  // collection.
  auto &result = I.getResult();
  if (Type::value_is_object(result)) {
    this->set_reaching_definition(&result, &result);
  }

  return;
}

void SSAConstructionVisitor::visitUsePHIInst(UsePHIInst &I) {
  return;
}

void SSAConstructionVisitor::visitArgPHIInst(ArgPHIInst &I) {
  return;
}

void SSAConstructionVisitor::visitRetPHIInst(RetPHIInst &I) {
  return;
}

// Update instructions
void SSAConstructionVisitor::visitMutWriteInst(MutWriteInst &I) {
  MemOIRBuilder builder(I);

  // Split the live range of the collection being written.
  auto *orig = &I.getObject();
  auto *curr = update_reaching_definition(orig, I);

  // Collect the extra arguments for the write instruction.
  vector<llvm::Value *> arguments(I.indices_begin(), I.indices_end());

  // Create IndexWriteInst.
  auto *redef = builder.CreateWriteInst(I.getElementType(),
                                        &I.getValueWritten(),
                                        curr,
                                        arguments);

  // Update the reaching definitions.
  this->set_reaching_definition(orig, redef);
  this->set_reaching_definition(redef, curr);

  // Mark old instruction for cleanup.
  this->mark_for_cleanup(I);

  return;
}

void SSAConstructionVisitor::visitMutInsertInst(MutInsertInst &I) {
  MemOIRBuilder builder(I);

  // Fetch operand information.
  auto *orig = &I.getObject();
  auto *curr = update_reaching_definition(orig, I);

  // Handle keywords.
  this->prepare_keywords(builder, I, &I.getElementType());

  // Collect the arguments.
  vector<llvm::Value *> arguments(
      llvm::User::value_op_iterator(std::next(&I.getObjectAsUse())),
      llvm::User::value_op_iterator(I.getCallInst().arg_end()));

  // Create SeqInsertInst.
  auto *redef = builder.CreateInsertInst(curr, arguments);

  // Update reaching definitions.
  this->set_reaching_definition(orig, redef);
  this->set_reaching_definition(redef, curr);

  // Mark old instruction for cleanup.
  this->mark_for_cleanup(I);

  return;
}

void SSAConstructionVisitor::visitMutRemoveInst(MutRemoveInst &I) {
  MemOIRBuilder builder(I);

  // Fetch the reaching definition.
  auto *orig = &I.getObject();
  auto *curr = update_reaching_definition(orig, I);

  this->prepare_keywords(builder, I, &I.getElementType());

  vector<llvm::Value *> arguments(
      llvm::User::value_op_iterator(std::next(&I.getObjectAsUse())),
      llvm::User::value_op_iterator(I.getCallInst().arg_end()));

  auto *redef = builder.CreateRemoveInst(curr, arguments);

  // Update reaching definitions.
  this->set_reaching_definition(orig, redef);
  this->set_reaching_definition(redef, curr);

  // Mark old instruction for cleanup.
  this->mark_for_cleanup(I);

  return;
}

void SSAConstructionVisitor::visitMutClearInst(MutClearInst &I) {
  MemOIRBuilder builder(I);

  // Split the live range of the collection being written.
  auto *orig = &I.getObject();
  auto *curr = update_reaching_definition(orig, I);

  vector<llvm::Value *> arguments(I.indices_begin(), I.indices_end());

  auto *redef = builder.CreateClearInst(curr, arguments);

  // Update the reaching definitions.
  this->set_reaching_definition(orig, redef);
  this->set_reaching_definition(redef, curr);

  // Mark old instruction for cleanup.
  this->mark_for_cleanup(I);

  return;
}

// Access instructions
void SSAConstructionVisitor::visitReadInst(ReadInst &I) {
  // Split the live range of the collection being read.
  auto *orig = &I.getObject();
  auto *curr = update_reaching_definition(orig, I);

  // Update the read to operate on the reaching definition.
  I.getObjectAsUse().set(curr);

  if (this->construct_use_phis) {
    MemOIRBuilder builder(I, true);

    // Build a UsePHI for the instruction.
    auto *use_phi = builder.CreateUsePHI(curr);
    auto *use_phi_value = &use_phi->getResultCollection();

    // Update the reaching definitions.
    this->set_reaching_definition(orig, use_phi_value);
    this->set_reaching_definition(use_phi_value, curr);
  }

  return;
}

void SSAConstructionVisitor::visitGetInst(GetInst &I) {
  // Split the live range of the collection being read.
  auto *orig = &I.getObject();
  auto *curr = update_reaching_definition(orig, I);

  // Update the read to operate on the reaching definition.
  I.getObjectAsUse().set(curr);

  if (this->construct_use_phis) {
    MemOIRBuilder builder(I, true);

    // Build a UsePHI for the instruction.
    auto *use_phi = builder.CreateUsePHI(curr);
    auto *use_phi_value = &use_phi->getResultCollection();

    // Update the reaching definitions.
    this->set_reaching_definition(orig, use_phi_value);
    this->set_reaching_definition(use_phi_value, curr);
  }

  return;
}

void SSAConstructionVisitor::visitHasInst(HasInst &I) {
  // Split the live range of the collection being read.
  auto *orig = &I.getObject();
  auto *curr = update_reaching_definition(orig, I);

  // Update the read to operate on the reaching definition.
  I.getObjectAsUse().set(curr);

  if (this->construct_use_phis) {
    MemOIRBuilder builder(I, true);

    // Build a UsePHI for the instruction.
    auto *use_phi = builder.CreateUsePHI(curr);
    auto *use_phi_value = &use_phi->getResultCollection();

    // Update the reaching definitions.
    this->set_reaching_definition(orig, use_phi_value);
    this->set_reaching_definition(use_phi_value, curr);
  }

  return;
}

void SSAConstructionVisitor::cleanup() {
  MemOIRInst::invalidate();

  for (auto *inst : instructions_to_delete) {
    infoln(*inst);
    inst->eraseFromParent();
  }
}

} // namespace llvm::memoir
