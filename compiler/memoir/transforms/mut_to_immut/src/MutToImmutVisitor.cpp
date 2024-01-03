#include "MutToImmutVisitor.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/support/Casting.hpp"
#include "memoir/support/Print.hpp"

#include "memoir/analysis/TypeAnalysis.hpp"

namespace llvm::memoir {

llvm::Value *MutToImmutVisitor::update_reaching_definition(
    llvm::Value *variable,
    MemOIRInst &I) {
  return this->update_reaching_definition(variable, I.getCallInst());
}

llvm::Value *MutToImmutVisitor::update_reaching_definition(
    llvm::Value *variable,
    llvm::Instruction &I) {
  return this->update_reaching_definition(variable, &I);
}

llvm::Value *MutToImmutVisitor::update_reaching_definition(
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

void MutToImmutVisitor::set_reaching_definition(
    llvm::Value *variable,
    llvm::Value *reaching_definition) {
  this->reaching_definitions[variable] = reaching_definition;
}

void MutToImmutVisitor::set_reaching_definition(
    llvm::Value *variable,
    MemOIRInst *reaching_definition) {
  this->set_reaching_definition(variable, &reaching_definition->getCallInst());
}

void MutToImmutVisitor::set_reaching_definition(
    MemOIRInst *variable,
    llvm::Value *reaching_definition) {
  this->set_reaching_definition(&variable->getCallInst(), reaching_definition);
}

void MutToImmutVisitor::set_reaching_definition(
    MemOIRInst *variable,
    MemOIRInst *reaching_definition) {
  this->set_reaching_definition(&variable->getCallInst(),
                                &reaching_definition->getCallInst());
}

void MutToImmutVisitor::mark_for_cleanup(llvm::Instruction &I) {
  this->instructions_to_delete.insert(&I);
}

void MutToImmutVisitor::mark_for_cleanup(MemOIRInst &I) {
  this->mark_for_cleanup(I.getCallInst());
}

MutToImmutVisitor::MutToImmutVisitor(
    arcana::noelle::DominatorForest &DT,
    ordered_set<llvm::Value *> memoir_names,
    map<llvm::PHINode *, llvm::Value *> inserted_phis,
    MutToImmutStats *stats)
  : DT(DT),
    inserted_phis(inserted_phis),
    stats(stats) {
  this->reaching_definitions = {};
  for (auto *name : memoir_names) {
    this->set_reaching_definition(name, name);
  }
}

void MutToImmutVisitor::visitInstruction(llvm::Instruction &I) {
  for (auto &operand_use : I.operands()) {
    auto *operand_value = operand_use.get();
    if (!Type::value_is_collection_type(*operand_value)) {
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

void MutToImmutVisitor::visitPHINode(llvm::PHINode &I) {
  auto found_inserted_phi = this->inserted_phis.find(&I);
  if (found_inserted_phi != this->inserted_phis.end()) {
    auto *named_variable = found_inserted_phi->second;
    auto *reaching_definition =
        this->update_reaching_definition(named_variable, I);

    // Update the reaching definition for the named variable.
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

void MutToImmutVisitor::visitUsePHIInst(UsePHIInst &I) {
  return;
}

void MutToImmutVisitor::visitDefPHIInst(DefPHIInst &I) {
  return;
}

void MutToImmutVisitor::visitArgPHIInst(ArgPHIInst &I) {
  return;
}

void MutToImmutVisitor::visitRetPHIInst(RetPHIInst &I) {
  return;
}

void MutToImmutVisitor::visitMutStructWriteInst(MutStructWriteInst &I) {
  // NOTE: this is currently a direct translation to StructWriteInst, when we
  // update to use FieldArrays explicitly, this is where they will need to be
  // constructed.
  MemOIRBuilder builder(I);

  // Fetch type information.
  auto *type = TypeAnalysis::analyze(I.getObjectOperand());
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
  auto *ssa_write = builder.CreateStructWriteInst(field_type,
                                                  write_value,
                                                  struct_value,
                                                  field_index);

  // Mark old instruction for cleanup.
  this->mark_for_cleanup(I);

  return;
}

void MutToImmutVisitor::visitMutIndexWriteInst(MutIndexWriteInst &I) {
  MemOIRBuilder builder(I);

  // Fetch type information.
  auto *type = TypeAnalysis::analyze(I.getObjectOperand());
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

void MutToImmutVisitor::visitMutAssocWriteInst(MutAssocWriteInst &I) {
  MemOIRBuilder builder(I);

  // Fetch type information.
  auto *type = TypeAnalysis::analyze(I.getObjectOperand());
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

void MutToImmutVisitor::visitIndexReadInst(IndexReadInst &I) {
  MemOIRBuilder builder(I, true);

  // Split the live range of the collection being read.
  auto *collection_orig = &I.getObjectOperand();
  auto *collection_value = update_reaching_definition(collection_orig, I);

  // Update the read to operate on the reaching definition.
  I.getObjectOperandAsUse().set(collection_value);

  // Build a UsePHI for the instruction.
  auto *use_phi = builder.CreateUsePHI(collection_value);
  auto *use_phi_value = &use_phi->getResultCollection();

  // Set the UseInst for the UsePHI.
  // use_phi->setUseInst(I);

  // Update the reaching definitions.
  this->set_reaching_definition(collection_orig, use_phi_value);
  this->set_reaching_definition(use_phi_value, collection_value);

  return;
}

void MutToImmutVisitor::visitAssocReadInst(AssocReadInst &I) {
  MemOIRBuilder builder(I, true);

  // Split the live range of the collection being read.
  auto *collection_orig = &I.getObjectOperand();
  auto *collection_value = update_reaching_definition(collection_orig, I);

  // Update the read to operate on the reaching definition.
  I.getObjectOperandAsUse().set(collection_value);

  // Build a UsePHI for the instruction.
  auto *use_phi = builder.CreateUsePHI(collection_value);
  auto *use_phi_value = &use_phi->getResultCollection();

  // Set the UseInst for the UsePHI.
  // use_phi->setUseInst(I);

  // Update the reaching definitions.
  this->set_reaching_definition(collection_orig, use_phi_value);
  this->set_reaching_definition(use_phi_value, collection_value);

  return;
}

void MutToImmutVisitor::visitIndexGetInst(IndexGetInst &I) {
  MemOIRBuilder builder(I, true);

  // Split the live range of the collection being read.
  auto *collection_orig = &I.getObjectOperand();
  auto *collection_value = update_reaching_definition(collection_orig, I);

  // Update the read to operate on the reaching definition.
  I.getObjectOperandAsUse().set(collection_value);

  // Build a UsePHI for the instruction.
  auto *use_phi = builder.CreateUsePHI(collection_value);
  auto *use_phi_value = &use_phi->getResultCollection();

  // Set the UseInst for the UsePHI.
  // use_phi->setUseInst(I);

  // Update the reaching definitions.
  this->set_reaching_definition(collection_orig, use_phi_value);
  this->set_reaching_definition(use_phi_value, collection_value);

  return;
}

void MutToImmutVisitor::visitAssocGetInst(AssocGetInst &I) {
  MemOIRBuilder builder(I, true);

  // Split the live range of the collection being read.
  auto *collection_orig = &I.getObjectOperand();
  auto *collection_value = update_reaching_definition(collection_orig, I);

  // Update the read to operate on the reaching definition.
  I.getObjectOperandAsUse().set(collection_value);

  // Build a UsePHI for the instruction.
  auto *use_phi = builder.CreateUsePHI(collection_value);
  auto *use_phi_value = &use_phi->getResultCollection();

  // Set the UseInst for the UsePHI.
  // use_phi->setUseInst(I);

  // Update the reaching definitions.
  this->set_reaching_definition(collection_orig, use_phi_value);
  this->set_reaching_definition(use_phi_value, collection_value);

  return;
}

void MutToImmutVisitor::visitMutSeqInsertInst(MutSeqInsertInst &I) {
  MemOIRBuilder builder(I);

  // Fetch type information.
  auto *type = TypeAnalysis::analyze(I.getCollection());
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

  // Create SeqInsertInst.
  auto *ssa_insert = builder.CreateSeqInsertInst(element_type,
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

void MutToImmutVisitor::visitMutSeqInsertSeqInst(MutSeqInsertSeqInst &I) {
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

void MutToImmutVisitor::visitMutSeqRemoveInst(MutSeqRemoveInst &I) {
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

void MutToImmutVisitor::visitMutSeqAppendInst(MutSeqAppendInst &I) {
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

void MutToImmutVisitor::visitMutSeqSwapInst(MutSeqSwapInst &I) {
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

void MutToImmutVisitor::visitMutSeqSplitInst(MutSeqSplitInst &I) {
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

void MutToImmutVisitor::visitAssocHasInst(AssocHasInst &I) {
  MemOIRBuilder builder(I, true);

  // Split the live range of the collection being written.
  auto *collection_orig = &I.getObjectOperand();
  auto *collection_value = update_reaching_definition(collection_orig, I);

  // Update the write to operate on the reaching definition.
  I.getObjectOperandAsUse().set(collection_value);

  // Build a DefPHI for the instruction.
  auto *use_phi = builder.CreateUsePHI(collection_value);
  auto *use_phi_value = &use_phi->getUsedCollection();

  // Update the reaching definitions.
  this->set_reaching_definition(collection_orig, use_phi_value);
  this->set_reaching_definition(use_phi_value, collection_value);

  return;
}

void MutToImmutVisitor::visitMutAssocRemoveInst(MutAssocRemoveInst &I) {
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

void MutToImmutVisitor::visitMutAssocInsertInst(MutAssocInsertInst &I) {
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

void MutToImmutVisitor::cleanup() {
  for (auto *inst : instructions_to_delete) {
    infoln(*inst);
    inst->eraseFromParent();
  }
}

} // namespace llvm::memoir
