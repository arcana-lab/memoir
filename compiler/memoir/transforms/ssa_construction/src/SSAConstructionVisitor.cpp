#include "SSAConstructionVisitor.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/support/Casting.hpp"
#include "memoir/support/Print.hpp"

#include "memoir/utility/Metadata.hpp"

namespace memoir {

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

  this->reaching_definitions[variable] = reaching_variable;

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
    if (not Type::value_is_collection_type(*operand_value)) {
      continue;
    }

    auto *reaching_operand = update_reaching_definition(operand_value, I);
    operand_use.set(reaching_operand);
  }

  if (Type::value_is_collection_type(I)) {
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
    if (not Type::value_is_collection_type(*arg_value)) {
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
  if (Type::value_is_collection_type(I)) {
    this->set_reaching_definition(&I, &I);
  }

  return;
}

void SSAConstructionVisitor::visitReturnInst(llvm::ReturnInst &I) {

  // If the returned value is a collection:
  auto *return_value = I.getReturnValue();
  if (return_value != nullptr
      and Type::value_is_collection_type(*return_value)) {

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
    if (not Type::value_is_collection_type(A)) {
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
  auto &collection_use = I.getCollectionAsUse();
  auto *reaching_collection =
      update_reaching_definition(collection_use.get(), I);
  collection_use.set(reaching_collection);

  // For each of the closed collections:
  for (unsigned closed_idx = 0; closed_idx < I.getNumberOfClosed();
       ++closed_idx) {
    auto &closed_use = I.getClosedAsUse(closed_idx);
    auto *closed = closed_use.get();

    // If the closed value is not a collection type, skip it.
    if (not Type::value_is_collection_type(*closed)) {
      continue;
    }

    // Update the use to use the current reaching definition.
    auto *reaching = update_reaching_definition(closed, I);
    closed_use.set(reaching);

    // Insert a RetPHI for the fold operation.
    MemOIRBuilder builder(I, true);
    auto *function = I.getCallInst().getCalledFunction();
    auto *ret_phi = builder.CreateRetPHI(reaching, function);
    auto *ret_phi_value = &ret_phi->getResultCollection();

    // Update the reaching definitions.
    this->set_reaching_definition(closed, ret_phi_value);
    this->set_reaching_definition(ret_phi_value, reaching);
  }

  // Create a new reaching definition for the returned value, if it's a MEMOIR
  // collection.
  auto &result = I.getResult();
  if (Type::value_is_collection_type(result)) {
    this->set_reaching_definition(&result, &result);
  }

  return;
}

void SSAConstructionVisitor::visitUsePHIInst(UsePHIInst &I) {
  return;
}

void SSAConstructionVisitor::visitDefPHIInst(DefPHIInst &I) {
  return;
}

void SSAConstructionVisitor::visitArgPHIInst(ArgPHIInst &I) {
  return;
}

void SSAConstructionVisitor::visitRetPHIInst(RetPHIInst &I) {
  return;
}

void SSAConstructionVisitor::visitMutStructWriteInst(MutStructWriteInst &I) {
  // NOTE: this is currently a direct translation to StructWriteInst, when we
  // update to use FieldArrays explicitly, this is where they will need to be
  // constructed.
  MemOIRBuilder builder(I);

  // Fetch type information.
  auto *type = type_of(I.getObjectOperand());
  MEMOIR_NULL_CHECK(type, "Couldn't determine type of MutStructWriteInst!");
  auto *struct_type = dyn_cast<StructType>(type);
  MEMOIR_NULL_CHECK(struct_type,
                    "MutStructWriteInst not operating on a struct type");
  auto &field_type = struct_type->getFieldType(I.getFieldIndex());

  // Split the live range of the collection being written.
  // NOTE: this is only necessary when we are using Field Arrays explicitly.

  // Fetch operand information.
  auto *struct_value = &I.getObjectOperand();
  auto *write_value = &I.getValueWritten();
  auto *field_index = &I.getFieldIndexOperand();

  // Create IndexWriteInst.
  builder.CreateStructWriteInst(field_type,
                                write_value,
                                struct_value,
                                field_index);

  // Mark old instruction for cleanup.
  this->mark_for_cleanup(I);

  return;
}

void SSAConstructionVisitor::visitMutIndexWriteInst(MutIndexWriteInst &I) {
  MemOIRBuilder builder(I);

  // Fetch type information.
  auto *type = type_of(I.getObjectOperand());
  MEMOIR_NULL_CHECK(type, "Couldn't determine type of seq_insert!");
  auto *collection_type = dyn_cast<CollectionType>(type);
  MEMOIR_NULL_CHECK(collection_type,
                    "seq_insert not operating on a collection type");
  auto &element_type = collection_type->getElementType();

  // Split the live range of the collection being written.
  auto *collection_orig = &I.getObjectOperand();
  auto *collection_value = update_reaching_definition(collection_orig, I);

  // Fetch operand information.
  auto *write_value = &I.getValueWritten();
  // TODO: update me to handle multi-dimensional access when we resupport them.
  auto *index_value = &I.getIndexOfDimension(0);

  // Create IndexWriteInst.
  auto *ssa_write = builder.CreateIndexWriteInst(element_type,
                                                 write_value,
                                                 collection_value,
                                                 index_value);

  // Update the reaching definitions.
  this->set_reaching_definition(collection_orig, ssa_write);
  this->set_reaching_definition(ssa_write, collection_value);

  // Mark old instruction for cleanup.
  this->mark_for_cleanup(I);

  return;
}

void SSAConstructionVisitor::visitMutAssocWriteInst(MutAssocWriteInst &I) {
  MemOIRBuilder builder(I);

  // Fetch type information.
  auto *type = type_of(I.getObjectOperand());
  MEMOIR_NULL_CHECK(type, "Couldn't determine type of seq_insert!");
  auto *collection_type = dyn_cast<CollectionType>(type);
  MEMOIR_NULL_CHECK(collection_type,
                    "seq_insert not operating on a collection type");
  auto &element_type = collection_type->getElementType();

  // Split the live range of the collection being written.
  auto *collection_orig = &I.getObjectOperand();
  auto *collection_value = update_reaching_definition(collection_orig, I);

  // Fetch operand information.
  auto *write_value = &I.getValueWritten();
  auto *key_value = &I.getKeyOperand();

  // Create AssocWriteInst.
  auto *def_phi_value = builder.CreateAssocWriteInst(element_type,
                                                     write_value,
                                                     collection_value,
                                                     key_value);

  // Update the reaching definitions.
  this->set_reaching_definition(collection_orig, def_phi_value);
  this->set_reaching_definition(def_phi_value, collection_value);

  // Mark old instruction for cleanup.
  this->mark_for_cleanup(I);

  return;
}

void SSAConstructionVisitor::visitIndexReadInst(IndexReadInst &I) {
  // Split the live range of the collection being read.
  auto *collection_orig = &I.getObjectOperand();
  auto *collection_value = update_reaching_definition(collection_orig, I);

  // Update the read to operate on the reaching definition.
  I.getObjectOperandAsUse().set(collection_value);

  if (this->construct_use_phis) {
    MemOIRBuilder builder(I, true);

    // Build a UsePHI for the instruction.
    auto *use_phi = builder.CreateUsePHI(collection_value);
    auto *use_phi_value = &use_phi->getResultCollection();

    // Set the UseInst for the UsePHI.
    // use_phi->setUseInst(I);

    // Update the reaching definitions.
    this->set_reaching_definition(collection_orig, use_phi_value);
    this->set_reaching_definition(use_phi_value, collection_value);
  }

  return;
}

void SSAConstructionVisitor::visitAssocReadInst(AssocReadInst &I) {
  // Split the live range of the collection being read.
  auto *collection_orig = &I.getObjectOperand();
  auto *collection_value = update_reaching_definition(collection_orig, I);

  // Update the read to operate on the reaching definition.
  I.getObjectOperandAsUse().set(collection_value);

  if (this->construct_use_phis) {
    MemOIRBuilder builder(I, true);

    // Build a UsePHI for the instruction.
    auto *use_phi = builder.CreateUsePHI(collection_value);
    auto *use_phi_value = &use_phi->getResultCollection();

    // Set the UseInst for the UsePHI.
    // use_phi->setUseInst(I);

    // Update the reaching definitions.
    this->set_reaching_definition(collection_orig, use_phi_value);
    this->set_reaching_definition(use_phi_value, collection_value);
  }

  return;
}

void SSAConstructionVisitor::visitIndexGetInst(IndexGetInst &I) {
  // Split the live range of the collection being read.
  auto *collection_orig = &I.getObjectOperand();
  auto *collection_value = update_reaching_definition(collection_orig, I);

  // Update the read to operate on the reaching definition.
  I.getObjectOperandAsUse().set(collection_value);

  if (this->construct_use_phis) {
    MemOIRBuilder builder(I, true);

    // Build a UsePHI for the instruction.
    auto *use_phi = builder.CreateUsePHI(collection_value);
    auto *use_phi_value = &use_phi->getResultCollection();

    // Set the UseInst for the UsePHI.
    // use_phi->setUseInst(I);

    // Update the reaching definitions.
    this->set_reaching_definition(collection_orig, use_phi_value);
    this->set_reaching_definition(use_phi_value, collection_value);
  }

  return;
}

void SSAConstructionVisitor::visitAssocGetInst(AssocGetInst &I) {
  // Split the live range of the collection being read.
  auto *collection_orig = &I.getObjectOperand();
  auto *collection_value = update_reaching_definition(collection_orig, I);

  // Update the read to operate on the reaching definition.
  I.getObjectOperandAsUse().set(collection_value);

  if (this->construct_use_phis) {
    MemOIRBuilder builder(I, true);

    // Build a UsePHI for the instruction.
    auto *use_phi = builder.CreateUsePHI(collection_value);
    auto *use_phi_value = &use_phi->getResultCollection();

    // Set the UseInst for the UsePHI.
    // use_phi->setUseInst(I);

    // Update the reaching definitions.
    this->set_reaching_definition(collection_orig, use_phi_value);
    this->set_reaching_definition(use_phi_value, collection_value);
  }

  return;
}

void SSAConstructionVisitor::visitMutSeqInsertInst(MutSeqInsertInst &I) {
  MemOIRBuilder builder(I);

  // Fetch type information.
  auto *type = type_of(I.getCollection());
  MEMOIR_NULL_CHECK(type, "Couldn't determine type of seq_insert!");
  auto *collection_type = dyn_cast<CollectionType>(type);
  MEMOIR_NULL_CHECK(collection_type,
                    "seq_insert not operating on a collection type");

  // Fetch operand information.
  auto *collection_orig = &I.getCollection();
  auto *collection_value = update_reaching_definition(collection_orig, I);
  auto *index_value = &I.getInsertionPoint();

  // Create SeqInsertInst.
  auto *ssa_insert = builder.CreateSeqInsertInst(collection_value, index_value);

  // Update reaching definitions.
  this->set_reaching_definition(collection_orig, ssa_insert);
  this->set_reaching_definition(ssa_insert, collection_value);

  // Mark old instruction for cleanup.
  this->mark_for_cleanup(I);

  return;
}

void SSAConstructionVisitor::visitMutSeqInsertValueInst(
    MutSeqInsertValueInst &I) {
  MemOIRBuilder builder(I);

  // Fetch type information.
  auto *type = type_of(I.getCollection());
  MEMOIR_NULL_CHECK(type, "Couldn't determine type of seq_insert!");
  auto *collection_type = dyn_cast<CollectionType>(type);
  MEMOIR_NULL_CHECK(collection_type,
                    "seq_insert not operating on a collection type");
  auto &element_type = collection_type->getElementType();

  // Fetch operand information.
  auto *collection_orig = &I.getCollection();
  auto *collection_value = update_reaching_definition(collection_orig, I);
  auto *write_value = &I.getValueInserted();
  auto *index_value = &I.getInsertionPoint();

  // Create SeqInsertValueInst.
  auto *ssa_insert = builder.CreateSeqInsertValueInst(element_type,
                                                      write_value,
                                                      collection_value,
                                                      index_value);

  // Update reaching definitions.
  this->set_reaching_definition(collection_orig, ssa_insert);
  this->set_reaching_definition(ssa_insert, collection_value);

  // Mark old instruction for cleanup.
  this->mark_for_cleanup(I);

  return;
}

void SSAConstructionVisitor::visitMutSeqInsertSeqInst(MutSeqInsertSeqInst &I) {
  MemOIRBuilder builder(I);

  auto *collection_orig = &I.getCollection();
  auto *collection_value = update_reaching_definition(collection_orig, I);
  auto *insert_orig = &I.getInsertedCollection();
  auto *insert_value = update_reaching_definition(insert_orig, I);
  auto *index_value = &I.getInsertionPoint();

  // Create SeqInsertSeqInst.
  auto *ssa_insert = builder.CreateSeqInsertSeqInst(insert_value,
                                                    collection_value,
                                                    index_value,
                                                    "seq.insert.");

  // Update reaching definitions.
  this->set_reaching_definition(collection_orig, ssa_insert);
  this->set_reaching_definition(ssa_insert, collection_value);

  // Mark old instructions for cleanup.
  this->mark_for_cleanup(I);

  return;
}

void SSAConstructionVisitor::visitMutSeqRemoveInst(MutSeqRemoveInst &I) {
  MemOIRBuilder builder(I);

  auto *collection_orig = &I.getCollection();
  auto *collection_value = update_reaching_definition(collection_orig, I);
  auto *begin_value = &I.getBeginIndex();
  auto *end_value = &I.getEndIndex();

  // Create SeqRemoveInst.
  auto *ssa_remove = builder.CreateSeqRemoveInst(collection_value,
                                                 begin_value,
                                                 end_value,
                                                 "seq.remove.");

  // Update reaching definitions.
  this->set_reaching_definition(collection_orig, ssa_remove);
  this->set_reaching_definition(ssa_remove, collection_value);

  // Mark old instruction for cleanup.
  this->mark_for_cleanup(I);

  return;
}

void SSAConstructionVisitor::visitMutSeqAppendInst(MutSeqAppendInst &I) {
  MemOIRBuilder builder(I);

  auto *collection_orig = &I.getCollection();
  auto *collection_value = update_reaching_definition(collection_orig, I);
  auto *appended_collection_orig = &I.getAppendedCollection();
  auto *appended_collection_value =
      update_reaching_definition(appended_collection_orig, I);

  // Create EndInst.
  auto *end = builder.CreateEndInst("seq.end.");

  // Create SeqInsertSeqInst.
  auto *ssa_append = builder.CreateSeqInsertSeqInst(appended_collection_value,
                                                    collection_value,
                                                    &end->getCallInst(),
                                                    "seq.append.");

  // Update reaching definitions.
  this->set_reaching_definition(collection_orig, ssa_append);
  this->set_reaching_definition(ssa_append, collection_value);

  // Mark old instruction for cleanup.
  this->mark_for_cleanup(I);

  return;
}

void SSAConstructionVisitor::visitMutSeqSwapInst(MutSeqSwapInst &I) {
  MemOIRBuilder builder(I);

  auto *from_collection_orig = &I.getFromCollection();
  auto *from_collection_value =
      update_reaching_definition(from_collection_orig, I);
  auto *from_begin_value = &I.getBeginIndex();
  auto *from_end_value = &I.getEndIndex();
  auto *to_collection_orig = &I.getToCollection();
  auto *to_collection_value = update_reaching_definition(to_collection_orig, I);
  auto *to_begin_value = &I.getToBeginIndex();

  // Create SeqSwapInst
  auto *ssa_swap = builder.CreateSeqSwapInst(from_collection_value,
                                             from_begin_value,
                                             from_end_value,
                                             to_collection_value,
                                             to_begin_value,
                                             "seq.swap.");

  // Extract the FROM and TO collections
  auto *ssa_from_collection =
      builder.CreateExtractValue(&ssa_swap->getCallInst(),
                                 llvm::ArrayRef<unsigned>({ 0 }),
                                 "seq.swap.from.");
  auto *ssa_to_collection =
      builder.CreateExtractValue(&ssa_swap->getCallInst(),
                                 llvm::ArrayRef<unsigned>({ 1 }),
                                 "seq.swap.to.");

  // Update the reaching definitions.
  this->set_reaching_definition(from_collection_orig, ssa_from_collection);
  this->set_reaching_definition(ssa_from_collection, from_collection_value);

  this->set_reaching_definition(to_collection_orig, ssa_to_collection);
  this->set_reaching_definition(ssa_to_collection, to_collection_value);

  // Mark instruction for cleanup.
  this->mark_for_cleanup(I);

  return;
}

void SSAConstructionVisitor::visitMutSeqSwapWithinInst(
    MutSeqSwapWithinInst &I) {
  MemOIRBuilder builder(I);

  auto *from_collection_orig = &I.getFromCollection();
  auto *from_collection_value =
      update_reaching_definition(from_collection_orig, I);
  auto *from_begin_value = &I.getBeginIndex();
  auto *from_end_value = &I.getEndIndex();
  auto *to_begin_value = &I.getToBeginIndex();

  // Create SeqSwapWithinInst
  auto *ssa_from_collection =
      builder.CreateSeqSwapWithinInst(from_collection_value,
                                      from_begin_value,
                                      from_end_value,
                                      to_begin_value,
                                      "seq.swap.");

  // Update the reaching definitions.
  this->set_reaching_definition(from_collection_orig, ssa_from_collection);
  this->set_reaching_definition(ssa_from_collection, from_collection_value);

  // Mark instruction for cleanup.
  this->mark_for_cleanup(I);

  return;
}

void SSAConstructionVisitor::visitMutSeqSplitInst(MutSeqSplitInst &I) {
  MemOIRBuilder builder(I);

  auto *split_value = &I.getSplit();
  auto *collection_orig = &I.getCollection();
  auto *collection_value = update_reaching_definition(collection_orig, I);
  auto *begin_value = &I.getBeginIndex();
  auto *end_value = &I.getEndIndex();

  // Create SeqCopy
  auto *ssa_split = builder.CreateSeqCopyInst(collection_value,
                                              begin_value,
                                              end_value,
                                              "seq.split.");

  // Create SeqRemove
  auto *ssa_remaining = builder.CreateSeqRemoveInst(collection_value,
                                                    begin_value,
                                                    end_value,
                                                    "seq.split.remaining.");

  // Update reaching definitions.
  this->set_reaching_definition(split_value, ssa_split);
  this->set_reaching_definition(collection_orig, ssa_remaining);
  this->set_reaching_definition(ssa_remaining, collection_value);

  // Mark old instruction for cleanup.
  this->mark_for_cleanup(I);

  return;
}

void SSAConstructionVisitor::visitAssocHasInst(AssocHasInst &I) {
  // Split the live range of the collection being accessed.
  auto *collection_orig = &I.getObjectOperand();
  auto *collection_value = update_reaching_definition(collection_orig, I);

  // Update the write to operate on the reaching definition.
  I.getObjectOperandAsUse().set(collection_value);

  if (this->construct_use_phis) {
    MemOIRBuilder builder(I, true);

    // Build a UsePHI for the instruction.
    auto *use_phi = builder.CreateUsePHI(collection_value);
    auto *use_phi_value = &use_phi->getUsedCollection();

    // Update the reaching definitions.
    this->set_reaching_definition(collection_orig, use_phi_value);
    this->set_reaching_definition(use_phi_value, collection_value);
  }

  return;
}

void SSAConstructionVisitor::visitMutAssocRemoveInst(MutAssocRemoveInst &I) {
  MemOIRBuilder builder(I);

  // Split the live range of the collection being written.
  auto *collection_orig = &I.getCollection();
  auto *collection_value = update_reaching_definition(collection_orig, I);

  // Update the write to operate on the reaching definition.
  I.getCollectionAsUse().set(collection_value);

  // Replace the MUT operation with its SSA form.
  auto *ssa_remove =
      builder.CreateAssocRemoveInst(collection_value, &I.getKeyOperand());
  auto *ssa_remove_value = &ssa_remove->getCallInst();

  // Update the reaching definitions.
  this->set_reaching_definition(collection_orig, ssa_remove_value);
  this->set_reaching_definition(ssa_remove_value, collection_value);

  this->mark_for_cleanup(I);

  return;
}

void SSAConstructionVisitor::visitMutAssocInsertInst(MutAssocInsertInst &I) {
  MemOIRBuilder builder(I);

  // Split the live range of the collection being written.
  auto *collection_orig = &I.getCollection();
  auto *collection_value = update_reaching_definition(collection_orig, I);

  // Update the write to operate on the reaching definition.
  I.getCollectionAsUse().set(collection_value);

  // Replace the MUT operation with its SSA form.
  auto *ssa_insert =
      builder.CreateAssocInsertInst(collection_value, &I.getKeyOperand());
  auto *ssa_insert_value = &ssa_insert->getCallInst();

  // Update the reaching definitions.
  this->set_reaching_definition(collection_orig, ssa_insert_value);
  this->set_reaching_definition(ssa_insert_value, collection_value);

  this->mark_for_cleanup(I);

  return;
}

void SSAConstructionVisitor::cleanup() {
  MemOIRInst::invalidate();

  for (auto *inst : instructions_to_delete) {
    infoln(*inst);
    inst->eraseFromParent();
  }
}

} // namespace memoir
