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

  // println("Computing reaching definition:");
  // println("  for", *variable);
  // println("  at ", *program_point);

  // println(*(program_point->getParent()));

  do {
    auto found_reaching_definition =
        this->reaching_definitions.find(reaching_variable);
    if (found_reaching_definition == this->reaching_definitions.end()) {
      reaching_variable = nullptr;
      break;
    }

    auto *next_reaching_variable = found_reaching_definition->second;

    // if (next_reaching_variable) {
    //   println("=> ", *next_reaching_variable);
    // } else {
    //   println("=> NULL");
    // }

    reaching_variable = next_reaching_variable;

    // If the reaching definition dominates the program point, update it.
    if (auto *reaching_definition =
            dyn_cast_or_null<llvm::Instruction>(reaching_variable)) {
      if (this->DT.dominates(reaching_definition, program_point)) {
        break;
      }
    }

    // Arguments dominate all program points in the function.
    if (auto *reaching_definition_as_argument =
            dyn_cast_or_null<llvm::Argument>(reaching_variable)) {
      break;
    }

  } while (reaching_variable != nullptr && reaching_variable != variable);

  this->reaching_definitions[variable] = reaching_variable;

  return reaching_variable;
}

MutToImmutVisitor::MutToImmutVisitor(
    llvm::noelle::DomTreeSummary &DT,
    ordered_set<llvm::Value *> memoir_names,
    map<llvm::PHINode *, llvm::Value *> inserted_phis,
    MutToImmutStats *stats)
  : DT(DT),
    inserted_phis(inserted_phis),
    stats(stats) {
  this->reaching_definitions = {};
  for (auto *name : memoir_names) {
    this->reaching_definitions[name] = name;
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
    this->reaching_definitions[&I] = &I;
  }

  return;
}

void MutToImmutVisitor::visitPHINode(llvm::PHINode &I) {
  auto found_inserted_phi = this->inserted_phis.find(&I);
  if (found_inserted_phi != this->inserted_phis.end()) {
    auto *named_variable = found_inserted_phi->second;
    auto *reaching_definition =
        this->update_reaching_definition(named_variable, I);
    this->reaching_definitions[&I] = reaching_definition;
    this->reaching_definitions[named_variable] = &I;
  } else {
    this->reaching_definitions[&I] = &I;
  }

  return;
}

void MutToImmutVisitor::visitUsePHIInst(UsePHIInst &I) {
  return;
}

void MutToImmutVisitor::visitDefPHIInst(DefPHIInst &I) {
  return;
}

void MutToImmutVisitor::visitIndexWriteInst(IndexWriteInst &I) {
  MemOIRBuilder builder(I, false);

  // Split the live range of the collection being written.
  auto *collection_orig = &I.getObjectOperand();
  auto *collection_value = update_reaching_definition(collection_orig, I);

  // Update the write to operate on the reaching definition.
  I.getObjectOperandAsUse().set(collection_value);

  // Build a DefPHI for the instruction.
  auto *def_phi = builder.CreateDefPHI(collection_value);
  auto *def_phi_value = &def_phi->getCollectionValue();

  // Set the DefInst for the UsePHI.
  // def_phi->setDefInst(I);

  // Update the reaching definitions.
  this->reaching_definitions[collection_orig] = def_phi_value;
  this->reaching_definitions[def_phi_value] = collection_value;

  return;
}

void MutToImmutVisitor::visitAssocWriteInst(AssocWriteInst &I) {
  MemOIRBuilder builder(I, false);

  // Split the live range of the collection being written.
  auto *collection_orig = &I.getObjectOperand();
  auto *collection_value = update_reaching_definition(collection_orig, I);

  // Update the write to operate on the reaching definition.
  I.getObjectOperandAsUse().set(collection_value);

  // Build a DefPHI for the instruction.
  auto *def_phi = builder.CreateDefPHI(collection_value);
  auto *def_phi_value = &def_phi->getCollectionValue();

  // Set the DefInst for the UsePHI.
  // def_phi->setDefInst(I);

  // Update the reaching definitions.
  this->reaching_definitions[collection_orig] = def_phi_value;
  this->reaching_definitions[def_phi_value] = collection_value;

  return;
}

void MutToImmutVisitor::visitIndexReadInst(IndexReadInst &I) {
  MemOIRBuilder builder(I, false);

  // Split the live range of the collection being read.
  auto *collection_orig = &I.getObjectOperand();
  auto *collection_value = update_reaching_definition(collection_orig, I);

  // Update the read to operate on the reaching definition.
  I.getObjectOperandAsUse().set(collection_value);

  // Build a DefPHI for the instruction.
  auto *use_phi = builder.CreateUsePHI(collection_value);
  auto *use_phi_value = &use_phi->getCollectionValue();

  // Set the UseInst for the UsePHI.
  // use_phi->setUseInst(I);

  // Update the reaching definitions.
  this->reaching_definitions[collection_orig] = use_phi_value;
  this->reaching_definitions[use_phi_value] = collection_value;

  return;
}

void MutToImmutVisitor::visitAssocReadInst(AssocReadInst &I) {
  MemOIRBuilder builder(I, false);

  // Split the live range of the collection being read.
  auto *collection_orig = &I.getObjectOperand();
  auto *collection_value = update_reaching_definition(collection_orig, I);

  // Update the read to operate on the reaching definition.
  I.getObjectOperandAsUse().set(collection_value);

  // Build a UsePHI for the instruction.
  auto *use_phi = builder.CreateUsePHI(collection_value);
  auto *use_phi_value = &use_phi->getCollectionValue();

  // Set the UseInst for the UsePHI.
  // use_phi->setUseInst(I);

  // Update the reaching definitions.
  this->reaching_definitions[collection_orig] = use_phi_value;
  this->reaching_definitions[use_phi_value] = collection_value;

  return;
}

void MutToImmutVisitor::visitIndexGetInst(IndexGetInst &I) {
  MemOIRBuilder builder(I, false);

  // Split the live range of the collection being read.
  auto *collection_orig = &I.getObjectOperand();
  auto *collection_value = update_reaching_definition(collection_orig, I);

  // Update the read to operate on the reaching definition.
  I.getObjectOperandAsUse().set(collection_value);

  // Build a UsePHI for the instruction.
  auto *use_phi = builder.CreateUsePHI(collection_value);
  auto *use_phi_value = &use_phi->getCollectionValue();

  // Set the UseInst for the UsePHI.
  // use_phi->setUseInst(I);

  // Update the reaching definitions.
  this->reaching_definitions[collection_orig] = use_phi_value;
  this->reaching_definitions[use_phi_value] = collection_value;

  return;
}

void MutToImmutVisitor::visitAssocGetInst(AssocGetInst &I) {
  MemOIRBuilder builder(I, false);

  // Split the live range of the collection being read.
  auto *collection_orig = &I.getObjectOperand();
  auto *collection_value = update_reaching_definition(collection_orig, I);

  // Update the read to operate on the reaching definition.
  I.getObjectOperandAsUse().set(collection_value);

  // Build a UsePHI for the instruction.
  auto *use_phi = builder.CreateUsePHI(collection_value);
  auto *use_phi_value = &use_phi->getCollectionValue();

  // Set the UseInst for the UsePHI.
  // use_phi->setUseInst(I);

  // Update the reaching definitions.
  this->reaching_definitions[collection_orig] = use_phi_value;
  this->reaching_definitions[use_phi_value] = collection_value;

  return;
}

void MutToImmutVisitor::visitSeqInsertInst(SeqInsertInst &I) {
  MemOIRBuilder builder(I);

  auto *type = TypeAnalysis::analyze(I.getCollectionOperand());
  MEMOIR_NULL_CHECK(type, "Couldn't determine type of seq_insert!");
  auto *collection_type = dyn_cast<CollectionType>(type);
  MEMOIR_NULL_CHECK(collection_type,
                    "seq_insert not operating on a collection type");
  auto &element_type = collection_type->getElementType();
  auto *collection_orig = &I.getCollectionOperand();
  auto *collection_value = update_reaching_definition(collection_orig, I);
  auto *write_value = &I.getValueInserted();
  auto *index_value = &I.getIndex();

  auto *elem_alloc =
      &builder.CreateSequenceAllocInst(element_type, 1, "insert.elem.")
           ->getCallInst();
  auto *elem_write = &builder
                          .CreateIndexWriteInst(element_type,
                                                write_value,
                                                elem_alloc,
                                                builder.getInt64(0))
                          ->getCallInst();

  if (auto *index_as_const_int = dyn_cast<llvm::ConstantInt>(index_value)) {
    if (index_as_const_int->isZero()) {
      // We only need to create a join to the front.
      auto *push_front_join = &builder
                                   .CreateJoinInst(vector<llvm::Value *>(
                                       { collection_value, elem_alloc }))
                                   ->getCallInst();

      this->reaching_definitions[collection_orig] = push_front_join;
      this->reaching_definitions[push_front_join] = collection_value;

      this->instructions_to_delete.insert(&I.getCallInst());
      return;
    }
  }

  // if (auto *index_as_size = dyn_cast<SizeInst>(&index_value)) {
  // TODO: if the index is a size(collection) and the collection has not been
  // modified in size since the call to size(collection), we can simply join
  // to the end.
  // }

  auto *left_slice = &builder
                          .CreateSliceInst(collection_value,
                                           (int64_t)0,
                                           index_value,
                                           "insert.left.")
                          ->getCallInst();
  auto *right_slice =
      &builder
           .CreateSliceInst(collection_value, index_value, -1, "insert.right.")
           ->getCallInst();

  auto *insert_join =
      &builder
           .CreateJoinInst(
               vector<llvm::Value *>({ left_slice, elem_alloc, right_slice }),
               "insert.join.")
           ->getCallInst();

  auto *delete_elem =
      &builder.CreateDeleteCollectionInst(elem_alloc)->getCallInst();

  this->reaching_definitions[collection_orig] = insert_join;
  this->reaching_definitions[insert_join] = collection_value;

  this->instructions_to_delete.insert(&I.getCallInst());

  return;
}

void MutToImmutVisitor::visitSeqInsertSeqInst(SeqInsertSeqInst &I) {
  MemOIRBuilder builder(I);

  auto *collection_orig = &I.getCollectionOperand();
  auto *collection_value = update_reaching_definition(collection_orig, I);
  auto *insert_orig = &I.getInsertedOperand();
  auto *insert_value = update_reaching_definition(insert_orig, I);
  auto *index_value = &I.getIndex();

  if (auto *index_as_const_int = dyn_cast<llvm::ConstantInt>(index_value)) {
    if (index_as_const_int->isZero()) {
      // We only need to create a join to the front.
      auto *push_front_join = &builder
                                   .CreateJoinInst(vector<llvm::Value *>(
                                       { collection_value, insert_value }))
                                   ->getCallInst();

      this->reaching_definitions[collection_orig] = push_front_join;
      this->reaching_definitions[push_front_join] = collection_value;

      this->instructions_to_delete.insert(&I.getCallInst());
      return;
    }
  }

  // if (auto *index_as_size = dyn_cast<SizeInst>(&index_value)) {
  // TODO: if the index is a size(collection) and the collection has not been
  // modified in size since the call to size(collection), we can simply join
  // to the end.
  // }

  auto *left_slice = &builder
                          .CreateSliceInst(collection_value,
                                           (int64_t)0,
                                           index_value,
                                           "insert.left.")
                          ->getCallInst();
  auto *right_slice =
      &builder
           .CreateSliceInst(collection_value, index_value, -1, "insert.right.")
           ->getCallInst();

  auto *insert_join =
      &builder
           .CreateJoinInst(
               vector<llvm::Value *>({ left_slice, insert_value, right_slice }),
               "insert.join.")
           ->getCallInst();

  this->reaching_definitions[collection_orig] = insert_join;
  this->reaching_definitions[insert_join] = collection_value;

  this->instructions_to_delete.insert(&I.getCallInst());

  return;
}

void MutToImmutVisitor::visitSeqRemoveInst(SeqRemoveInst &I) {
  MemOIRBuilder builder(I);

  auto *collection_orig = &I.getCollectionOperand();
  auto *collection_value = update_reaching_definition(collection_orig, I);
  auto *begin_value = &I.getBeginIndex();
  auto *end_value = &I.getEndIndex();

  if (auto *begin_as_const_int = dyn_cast<llvm::ConstantInt>(begin_value)) {
    if (begin_as_const_int->isZero()) {
      // We only need to create a slice from [end_index:end)
      auto *pop_front_left = &builder
                                  .CreateSliceInst(collection_value,
                                                   (int64_t)0,
                                                   (int64_t)0,
                                                   "remove.pop_front.")
                                  ->getCallInst();
      auto *pop_front_right = &builder
                                   .CreateSliceInst(collection_value,
                                                    end_value,
                                                    -1,
                                                    "remove.pop_front.")
                                   ->getCallInst();

      auto *pop_front =
          &builder
               .CreateJoinInst(
                   vector<llvm::Value *>({ pop_front_left, pop_front_right }),
                   "remove.pop_front.")
               ->getCallInst();

      this->reaching_definitions[collection_orig] = pop_front;
      this->reaching_definitions[pop_front] = collection_value;

      this->instructions_to_delete.insert(&I.getCallInst());

      return;
    }
  }

  if (auto *end_as_const_int = dyn_cast<llvm::ConstantInt>(end_value)) {
    if (end_as_const_int->isMinusOne()) {
      // We only need to create a slice from [0:begin)
      auto *pop_back_left = &builder
                                 .CreateSliceInst(collection_value,
                                                  (int64_t)0,
                                                  begin_value,
                                                  "remove.pop_back.")
                                 ->getCallInst();
      auto *pop_back_right = &builder
                                  .CreateSliceInst(collection_value,
                                                   -1,
                                                   -1,
                                                   "remove.pop_back.rest")
                                  ->getCallInst();

      auto *pop_back =
          &builder
               .CreateJoinInst(
                   vector<llvm::Value *>({ pop_back_left, pop_back_right }),
                   "remove.pop_back.")
               ->getCallInst();

      this->reaching_definitions[collection_orig] = pop_back;
      this->reaching_definitions[pop_back] = collection_value;

      this->instructions_to_delete.insert(&I.getCallInst());

      return;
    }
  }

  auto *left_slice = &builder
                          .CreateSliceInst(collection_value,
                                           (int64_t)0,
                                           begin_value,
                                           "remove.left.")
                          ->getCallInst();
  auto *size =
      &builder.CreateSizeInst(collection_value, "remove.end.")->getCallInst();
  auto *right_slice =
      &builder
           .CreateSliceInst(collection_value, end_value, size, "remove.right.")
           ->getCallInst();

  auto *remove_join =
      &builder
           .CreateJoinInst(vector<llvm::Value *>({ left_slice, right_slice }),
                           "remove.join.")
           ->getCallInst();
  // TODO: attach metadata to this to say that begin < end

  this->reaching_definitions[collection_orig] = remove_join;
  this->reaching_definitions[remove_join] = collection_value;

  this->instructions_to_delete.insert(&I.getCallInst());
  return;
}

void MutToImmutVisitor::visitSeqAppendInst(SeqAppendInst &I) {
  MemOIRBuilder builder(I);

  auto *collection_orig = &I.getCollectionOperand();
  auto *collection_value = update_reaching_definition(collection_orig, I);
  auto *appended_collection_orig = &I.getAppendedCollectionOperand();
  auto *appended_collection_value =
      update_reaching_definition(appended_collection_orig, I);

  auto *append_join =
      &builder
           .CreateJoinInst(vector<llvm::Value *>(
                               { collection_value, appended_collection_value }),
                           "append.")
           ->getCallInst();

  this->reaching_definitions[collection_orig] = append_join;
  this->reaching_definitions[append_join] = collection_value;

  this->instructions_to_delete.insert(&I.getCallInst());
  return;
}

void MutToImmutVisitor::visitSeqSwapInst(SeqSwapInst &I) {
  MemOIRBuilder builder(I);

  auto *from_collection_orig = &I.getFromCollectionOperand();
  auto *from_collection_value =
      update_reaching_definition(from_collection_orig, I);
  auto *from_begin_value = &I.getBeginIndex();
  auto *from_end_value = &I.getEndIndex();
  auto *to_collection_orig = &I.getToCollectionOperand();
  auto *to_collection_value = update_reaching_definition(to_collection_orig, I);
  auto *to_begin_value = &I.getToBeginIndex();

  if (from_collection_value == to_collection_value) {
    auto *collection_value = from_collection_value;

    llvm::Value *from_size = nullptr, *to_end_value = nullptr;
    llvm::Value *left = nullptr, *from = nullptr, *middle = nullptr,
                *to = nullptr, *right = nullptr;

    if (auto *from_begin_as_const_int =
            dyn_cast<llvm::ConstantInt>(from_begin_value)) {
      if (from_begin_as_const_int->isZero()) {
        from_size = from_end_value;
        from = &builder
                    .CreateSliceInst(collection_value,
                                     (int64_t)0,
                                     from_end_value,
                                     "swap.from.")
                    ->getCallInst();
      }
    }

    if (from == nullptr) {
      from_size =
          builder.CreateSub(from_end_value, from_begin_value, "swap.size.");
      left = &builder
                  .CreateSliceInst(collection_value,
                                   (int64_t)0,
                                   from_begin_value,
                                   "swap.left.")
                  ->getCallInst();
      from = &builder
                  .CreateSliceInst(collection_value,
                                   from_begin_value,
                                   from_end_value,
                                   "swap.from.")
                  ->getCallInst();
    }

    to_end_value = builder.CreateAdd(to_begin_value, from_size, "swap.to.end.");

    if (from_end_value == to_begin_value) {
      to = &builder
                .CreateSliceInst(collection_value,
                                 to_begin_value,
                                 to_end_value,
                                 "swap.to.")
                ->getCallInst();
      right = &builder
                   .CreateSliceInst(collection_value,
                                    to_end_value,
                                    -1,
                                    "swap.right")
                   ->getCallInst();
    } else {
      middle = &builder
                    .CreateSliceInst(collection_value,
                                     from_end_value,
                                     to_begin_value,
                                     "swap.middle")
                    ->getCallInst();
      to = &builder
                .CreateSliceInst(collection_value,
                                 to_begin_value,
                                 to_end_value,
                                 "swap.to.")
                ->getCallInst();
      right = &builder
                   .CreateSliceInst(collection_value,
                                    to_end_value,
                                    -1,
                                    "swap.right")
                   ->getCallInst();
    }

    vector<llvm::Value *> collections_to_join;
    collections_to_join.reserve(5);
    if (left != nullptr) {
      collections_to_join.push_back(left);
    }
    collections_to_join.push_back(to);
    if (middle != nullptr) {
      collections_to_join.push_back(middle);
    }
    collections_to_join.push_back(from);
    if (right != nullptr) {
      collections_to_join.push_back(right);
    }

    llvm::Value *join =
        &builder.CreateJoinInst(collections_to_join, "swap.join.")
             ->getCallInst();

    this->reaching_definitions[from_collection_orig] = join;
    this->reaching_definitions[join] = collection_value;

    this->instructions_to_delete.insert(&I.getCallInst());
    return;
  }

  llvm::Value *from_size = nullptr, *to_end_value = nullptr;
  llvm::Value *from_left = nullptr, *from_swap = nullptr, *from_right = nullptr;
  llvm::Value *to_left = nullptr, *to_swap = nullptr, *to_right = nullptr;
  if (auto *from_begin_as_const_int =
          dyn_cast<llvm::ConstantInt>(from_begin_value)) {
    if (from_begin_as_const_int->isZero()) {
      // We only need to create a slice from [end_index:end)
      from_size = from_end_value;
      from_swap = &builder
                       .CreateSliceInst(from_collection_value,
                                        (int64_t)0,
                                        from_end_value,
                                        "swap.from.")
                       ->getCallInst();
      from_right = &builder
                        .CreateSliceInst(from_collection_value,
                                         from_end_value,
                                         -1,
                                         "swap.from.rest.")
                        ->getCallInst();
    }
  }

  if (from_swap == nullptr) {
    from_size =
        builder.CreateSub(from_end_value, from_begin_value, "swap.from.size.");
    from_left = &builder
                     .CreateSliceInst(from_collection_value,
                                      (int64_t)0,
                                      from_begin_value,
                                      "swap.from.left.")
                     ->getCallInst();
    from_swap = &builder
                     .CreateSliceInst(from_collection_value,
                                      from_begin_value,
                                      from_end_value,
                                      "swap.from.")
                     ->getCallInst();
    from_right = &builder
                      .CreateSliceInst(from_collection_value,
                                       from_end_value,
                                       -1,
                                       "swap.from.right.")
                      ->getCallInst();
  }

  if (auto *to_begin_as_const_int =
          dyn_cast<llvm::ConstantInt>(to_begin_value)) {
    if (to_begin_as_const_int->isZero()) {
      to_end_value = from_size;
      to_swap = &builder
                     .CreateSliceInst(to_collection_value,
                                      (int64_t)0,
                                      to_end_value,
                                      "swap.to.")
                     ->getCallInst();
      to_right = &builder
                      .CreateSliceInst(to_collection_value,
                                       to_end_value,
                                       -1,
                                       "swap.to.rest.")
                      ->getCallInst();
    }
  }

  if (to_swap == nullptr) {
    to_end_value = builder.CreateAdd(to_begin_value, from_size, "swap.to.end.");
    to_left = &builder
                   .CreateSliceInst(to_collection_value,
                                    (int64_t)0,
                                    to_begin_value,
                                    "swap.to.left.")
                   ->getCallInst();
    to_swap = &builder
                   .CreateSliceInst(to_collection_value,
                                    to_begin_value,
                                    to_end_value,
                                    "swap.to.")
                   ->getCallInst();
    to_right = &builder
                    .CreateSliceInst(to_collection_value,
                                     to_end_value,
                                     -1,
                                     "swap.to.right.")
                    ->getCallInst();
  }

  vector<llvm::Value *> from_incoming;
  from_incoming.reserve(3);
  if (from_left != nullptr) {
    from_incoming.push_back(from_left);
  }
  from_incoming.push_back(to_swap);
  if (from_right != nullptr) {
    from_incoming.push_back(from_right);
  }

  llvm::Value *from_join =
      &builder.CreateJoinInst(from_incoming, "swap.from.join.")->getCallInst();

  vector<llvm::Value *> to_incoming;
  to_incoming.reserve(3);
  if (to_left != nullptr) {
    to_incoming.push_back(to_left);
  }
  to_incoming.push_back(from_swap);
  if (to_right != nullptr) {
    to_incoming.push_back(to_right);
  }
  llvm::Value *to_join =
      &builder.CreateJoinInst(to_incoming, "swap.to.join.")->getCallInst();

  this->reaching_definitions[from_collection_orig] = from_join;
  this->reaching_definitions[from_join] = from_collection_value;
  this->reaching_definitions[to_collection_orig] = to_join;
  this->reaching_definitions[to_join] = to_collection_value;

  this->instructions_to_delete.insert(&I.getCallInst());
  return;
}

void MutToImmutVisitor::visitSeqSplitInst(SeqSplitInst &I) {
  MemOIRBuilder builder(I);

  auto *split_value = &I.getSplitValue();
  auto *collection_orig = &I.getCollectionOperand();
  auto *collection_value = update_reaching_definition(collection_orig, I);
  auto *begin_value = &I.getBeginIndex();
  auto *end_value = &I.getEndIndex();

  if (auto *begin_as_const_int = dyn_cast<llvm::ConstantInt>(begin_value)) {
    if (begin_as_const_int->isZero()) {
      auto *split = &builder
                         .CreateSliceInst(collection_value,
                                          (int64_t)0,
                                          end_value,
                                          "split.")
                         ->getCallInst();
      auto *remaining = &builder
                             .CreateSliceInst(collection_value,
                                              end_value,
                                              -1,
                                              "split.remaining.")
                             ->getCallInst();

      this->reaching_definitions[split_value] = split;
      this->reaching_definitions[collection_orig] = remaining;
      this->reaching_definitions[remaining] = collection_value;

      this->instructions_to_delete.insert(&I.getCallInst());

      return;
    }
  }

  auto *left = &builder
                    .CreateSliceInst(collection_value,
                                     (int64_t)0,
                                     begin_value,
                                     "split.left.")
                    ->getCallInst();
  auto *split =
      &builder
           .CreateSliceInst(collection_value, begin_value, end_value, "split.")
           ->getCallInst();
  auto *right =
      &builder.CreateSliceInst(collection_value, end_value, -1, "split.right.")
           ->getCallInst();

  auto *remaining = &builder
                         .CreateJoinInst(vector<llvm::Value *>({ left, right }),
                                         "split.remaining.")
                         ->getCallInst();

  this->reaching_definitions[split_value] = split;
  this->reaching_definitions[collection_orig] = remaining;
  this->reaching_definitions[remaining] = collection_value;

  this->instructions_to_delete.insert(&I.getCallInst());
  return;
}

void MutToImmutVisitor::visitAssocHasInst(AssocHasInst &I) {
  MemOIRBuilder builder(I, false);

  // Split the live range of the collection being written.
  auto *collection_orig = &I.getObjectOperand();
  auto *collection_value = update_reaching_definition(collection_orig, I);

  // Update the write to operate on the reaching definition.
  I.getObjectOperandAsUse().set(collection_value);

  // Build a DefPHI for the instruction.
  auto *use_phi = builder.CreateUsePHI(collection_value);
  auto *use_phi_value = &use_phi->getCollectionValue();

  // Set the DefInst for the UsePHI.
  // def_phi->setDefInst(I);

  // Update the reaching definitions.
  this->reaching_definitions[collection_orig] = use_phi_value;
  this->reaching_definitions[use_phi_value] = collection_value;

  return;
}

void MutToImmutVisitor::visitAssocRemoveInst(AssocRemoveInst &I) {
  MemOIRBuilder builder(I, false);

  // Split the live range of the collection being written.
  auto *collection_orig = &I.getCollectionOperand();
  auto *collection_value = update_reaching_definition(collection_orig, I);

  // Update the write to operate on the reaching definition.
  I.getCollectionOperandAsUse().set(collection_value);

  // Build a DefPHI for the instruction.
  auto *def_phi = builder.CreateDefPHI(collection_value);
  auto *def_phi_value = &def_phi->getCollectionValue();

  // Set the DefInst for the UsePHI.
  // def_phi->setDefInst(I);

  // Update the reaching definitions.
  this->reaching_definitions[collection_orig] = def_phi_value;
  this->reaching_definitions[def_phi_value] = collection_value;

  return;
}

void MutToImmutVisitor::cleanup() {
  for (auto *inst : instructions_to_delete) {
    infoln(*inst);
    inst->eraseFromParent();
  }
}

} // namespace llvm::memoir
