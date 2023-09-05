#include "memoir/utility/FunctionNames.hpp"

#include "memoir/support/Assert.hpp"
#include "memoir/support/Casting.hpp"
#include "memoir/support/Print.hpp"

#include "memoir/analysis/TypeAnalysis.hpp"

#include "SSADestruction.hpp"

#define USE_VECTOR 1
#if !defined(USE_VECTOR)
#  define USE_VECTOR 0
#endif

namespace llvm::memoir {

SSADestructionVisitor::SSADestructionVisitor(llvm::noelle::DomTreeSummary &DT,
                                             LivenessAnalysis &LA,
                                             ValueNumbering &VN,
                                             SSADestructionStats *stats)
  : DT(DT),
    LA(LA),
    VN(VN),
    stats(stats) {
  // Do nothing.
}

void SSADestructionVisitor::visitInstruction(llvm::Instruction &I) {
  return;
}

void SSADestructionVisitor::visitSequenceAllocInst(SequenceAllocInst &I) {
#if USE_VECTOR
  auto &element_type = I.getElementType();

  auto element_code = element_type.get_code();
  auto vector_alloc_name = *element_code + "_vector__allocate";

  auto &M = MEMOIR_SANITIZE(I.getModule(), "Couldn't get module");
  auto *function = M.getFunction(vector_alloc_name);
  auto function_callee = FunctionCallee(function);
  if (function == nullptr) {
    println("Couldn't find vector alloc for ", vector_alloc_name);
    MEMOIR_UNREACHABLE("see above");
  }

  MemOIRBuilder builder(I);

  auto *function_type = function_callee.getFunctionType();
  auto *vector_size = &I.getSizeOperand();

  auto *llvm_call =
      builder.CreateCall(function_callee, llvm::ArrayRef({ vector_size }));
  MEMOIR_NULL_CHECK(llvm_call, "Could not create the call for vector read");

  auto *collection =
      builder.CreatePointerCast(llvm_call, I.getCallInst().getType());

  this->coalesce(I, *collection);
  this->markForCleanup(I);
#endif
  return;
}

void SSADestructionVisitor::visitAssocArrayAllocInst(AssocArrayAllocInst &I) {
#if USE_VECTOR
  auto &key_type = I.getKeyType();
  auto &value_type = I.getValueType();

  auto key_code = key_type.get_code();
  auto value_code = value_type.get_code();
  auto name = *key_code + "_" + *value_code + "_hashtable__allocate";

  auto &M = MEMOIR_SANITIZE(I.getModule(), "Couldn't get module");
  auto *function = M.getFunction(name);
  auto function_callee = FunctionCallee(function);
  if (function == nullptr) {
    println("Couldn't find assoc alloc for ", name);
    MEMOIR_UNREACHABLE("see above");
  }

  MemOIRBuilder builder(I);

  auto *function_type = function_callee.getFunctionType();

  auto *llvm_call = builder.CreateCall(function_callee);
  MEMOIR_NULL_CHECK(llvm_call, "Could not create the call for hashtable alloc");

  auto *collection =
      builder.CreatePointerCast(llvm_call, I.getCallInst().getType());

  this->coalesce(I, *collection);
  this->markForCleanup(I);
#endif
  return;
}

void SSADestructionVisitor::visitDeleteCollectionInst(DeleteCollectionInst &I) {
#if USE_VECTOR
  auto &collection_type =
      MEMOIR_SANITIZE(dyn_cast_or_null<CollectionType>(
                          TypeAnalysis::analyze(I.getCollectionOperand())),
                      "Couldn't determine type of collection");
  if (auto *seq_type = dyn_cast<SequenceType>(&collection_type)) {
    auto &element_type = seq_type->getElementType();

    auto element_code = element_type.get_code();
    auto vector_free_name = *element_code + "_vector__free";

    auto &M = MEMOIR_SANITIZE(I.getModule(), "Couldn't get module");
    auto *function = M.getFunction(vector_free_name);
    auto function_callee = FunctionCallee(function);
    if (function == nullptr) {
      warnln("Couldn't find vector free for ", vector_free_name);
      return;
    }

    MemOIRBuilder builder(I);

    auto *function_type = function_callee.getFunctionType();
    auto *vector_value =
        builder.CreatePointerCast(&I.getCollectionOperand(),
                                  function_type->getParamType(0));
    auto *llvm_call =
        builder.CreateCall(function_callee, llvm::ArrayRef({ vector_value }));
    MEMOIR_NULL_CHECK(llvm_call, "Could not create the call for vector read");

    this->markForCleanup(I);
  } else if (auto *assoc_type = dyn_cast<AssocArrayType>(&collection_type)) {
    auto &key_type = assoc_type->getKeyType();
    auto &value_type = assoc_type->getValueType();

    auto key_code = key_type.get_code();
    auto value_code = value_type.get_code();
    auto assoc_free_name = *key_code + "_" + *value_code + "_hashtable__free";

    auto &M = MEMOIR_SANITIZE(I.getModule(), "Couldn't get module");
    auto *function = M.getFunction(assoc_free_name);
    auto function_callee = FunctionCallee(function);
    if (function == nullptr) {
      warnln("Couldn't find assoc free for ", assoc_free_name);
      return;
    }

    MemOIRBuilder builder(I);

    auto *function_type = function_callee.getFunctionType();
    auto *assoc_value =
        builder.CreatePointerCast(&I.getCollectionOperand(),
                                  function_type->getParamType(0));
    auto *llvm_call =
        builder.CreateCall(function_callee, llvm::ArrayRef({ assoc_value }));
    MEMOIR_NULL_CHECK(llvm_call, "Could not create the call for assoc read");

    this->markForCleanup(I);
  }
#endif
  return;
}

void SSADestructionVisitor::visitSizeInst(SizeInst &I) {
#if USE_VECTOR
  auto &collection_type =
      MEMOIR_SANITIZE(dyn_cast_or_null<CollectionType>(
                          TypeAnalysis::analyze(I.getCollectionOperand())),
                      "Couldn't determine type of collection");
  std::string name;
  if (auto *seq_type = dyn_cast<SequenceType>(&collection_type)) {
    auto &element_type = seq_type->getElementType();

    auto element_code = element_type.get_code();
    name = *element_code + "_vector__size";
  } else if (auto *assoc_type = dyn_cast<AssocArrayType>(&collection_type)) {
    auto &key_type = assoc_type->getKeyType();
    auto &value_type = assoc_type->getValueType();

    auto key_code = key_type.get_code();
    auto value_code = value_type.get_code();
    name = *key_code + "_" + *value_code + "_hashtable__size";
  }

  auto &M = MEMOIR_SANITIZE(I.getModule(), "Couldn't get module");
  auto *function = M.getFunction(name);
  auto function_callee = FunctionCallee(function);
  if (function == nullptr) {
    warnln("Couldn't find size for ", name);
    return;
  }

  MemOIRBuilder builder(I);

  auto *function_type = function_callee.getFunctionType();
  auto *value = builder.CreatePointerCast(&I.getCollectionOperand(),
                                          function_type->getParamType(0));
  auto *llvm_call =
      builder.CreateCall(function_callee, llvm::ArrayRef({ value }));
  MEMOIR_NULL_CHECK(llvm_call, "Could not create the call for size");

  I.getCallInst().replaceAllUsesWith(llvm_call);
  this->markForCleanup(I);
#else
#endif
  return;
}

void SSADestructionVisitor::visitIndexReadInst(IndexReadInst &I) {
#if USE_VECTOR
  auto &collection_type =
      MEMOIR_SANITIZE(dyn_cast_or_null<CollectionType>(
                          TypeAnalysis::analyze(I.getObjectOperand())),
                      "Couldn't determine type of read collection");

  auto &element_type = collection_type.getElementType();

  auto element_code = element_type.get_code();
  auto vector_read_name = *element_code + "_vector__read";

  auto &M = MEMOIR_SANITIZE(I.getModule(), "Couldn't get module");
  auto *function = M.getFunction(vector_read_name);
  auto function_callee = FunctionCallee(function);
  if (function == nullptr) {
    println("Couldn't find vector read name for ", vector_read_name);
    MEMOIR_UNREACHABLE("see above");
  }

  MemOIRBuilder builder(I);

  auto *function_type = function_callee.getFunctionType();
  auto *vector_value =
      builder.CreatePointerCast(&I.getObjectOperand(),
                                function_type->getParamType(0));
  auto *vector_index =
      builder.CreateZExtOrBitCast(&I.getIndexOfDimension(0),
                                  function_type->getParamType(1));

  auto *llvm_call =
      builder.CreateCall(function_callee,
                         llvm::ArrayRef({ vector_value, vector_index }));
  MEMOIR_NULL_CHECK(llvm_call, "Could not create the call for vector read");

  I.getCallInst().replaceAllUsesWith(llvm_call);
  this->markForCleanup(I);
#endif
  return;
}

void SSADestructionVisitor::visitIndexWriteInst(IndexWriteInst &I) {
#if USE_VECTOR
  auto &collection_type =
      MEMOIR_SANITIZE(dyn_cast_or_null<CollectionType>(
                          TypeAnalysis::analyze(I.getObjectOperand())),
                      "Couldn't determine type of written collection");

  auto &element_type = collection_type.getElementType();

  auto element_code = element_type.get_code();
  auto vector_write_name = *element_code + "_vector__write";

  auto &M = MEMOIR_SANITIZE(I.getModule(), "Couldn't get module");
  auto *function = M.getFunction(vector_write_name);
  auto function_callee = FunctionCallee(function);
  if (function == nullptr) {
    println("Couldn't find vector write name for ", vector_write_name);
    MEMOIR_UNREACHABLE("see above");
  }

  MemOIRBuilder builder(I);

  auto *function_type = function_callee.getFunctionType();
  auto *vector_value =
      builder.CreatePointerCast(&I.getObjectOperand(),
                                function_type->getParamType(0));
  auto *vector_index =
      builder.CreateZExtOrBitCast(&I.getIndexOfDimension(0),
                                  function_type->getParamType(1));
  auto *write_value = &I.getValueWritten();

  auto *llvm_call = builder.CreateCall(
      function_callee,
      llvm::ArrayRef({ vector_value, vector_index, write_value }));
  MEMOIR_NULL_CHECK(llvm_call, "Could not create the call for vector write");

  this->markForCleanup(I);

#endif
  return;
}

void SSADestructionVisitor::visitAssocReadInst(AssocReadInst &I) {
#if USE_VECTOR
  auto &assoc_type =
      MEMOIR_SANITIZE(dyn_cast_or_null<AssocArrayType>(
                          TypeAnalysis::analyze(I.getObjectOperand())),
                      "Couldn't determine type of read collection");

  auto &key_type = assoc_type.getKeyType();
  auto &value_type = assoc_type.getValueType();

  auto key_code = key_type.get_code();
  auto value_code = value_type.get_code();
  auto name = *key_code + "_" + *value_code + "_hashtable__read";

  auto &M = MEMOIR_SANITIZE(I.getModule(), "Couldn't get module");
  auto *function = M.getFunction(name);
  auto function_callee = FunctionCallee(function);
  if (function == nullptr) {
    warnln("Couldn't find AssocRead for ", name);
    return;
  }

  MemOIRBuilder builder(I);

  auto *function_type = function_callee.getFunctionType();
  auto *assoc_value = builder.CreatePointerCast(&I.getObjectOperand(),
                                                function_type->getParamType(0));
  auto *assoc_key =
      builder.CreateBitOrPointerCast(&I.getKeyOperand(),
                                     function_type->getParamType(1));

  auto *llvm_call =
      builder.CreateCall(function_callee,
                         llvm::ArrayRef({ assoc_value, assoc_key }));
  MEMOIR_NULL_CHECK(llvm_call, "Could not create the call for AssocRead");

  I.getCallInst().replaceAllUsesWith(llvm_call);
  this->markForCleanup(I);
#endif
  return;
}

void SSADestructionVisitor::visitAssocWriteInst(AssocWriteInst &I) {
#if USE_VECTOR
  auto &assoc_type =
      MEMOIR_SANITIZE(dyn_cast_or_null<AssocArrayType>(
                          TypeAnalysis::analyze(I.getObjectOperand())),
                      "Couldn't determine type of written collection");

  auto &key_type = assoc_type.getKeyType();
  auto &value_type = assoc_type.getValueType();

  auto key_code = key_type.get_code();
  auto value_code = value_type.get_code();
  auto name = *key_code + "_" + *value_code + "_hashtable__write";

  auto &M = MEMOIR_SANITIZE(I.getModule(), "Couldn't get module");
  auto *function = M.getFunction(name);
  auto function_callee = FunctionCallee(function);
  if (function == nullptr) {
    warnln("Couldn't find AssocWrite name for ", name);
    return;
  }

  MemOIRBuilder builder(I);

  auto *function_type = function_callee.getFunctionType();
  auto *assoc_value = builder.CreatePointerCast(&I.getObjectOperand(),
                                                function_type->getParamType(0));
  auto *assoc_index =
      builder.CreateBitOrPointerCast(&I.getKeyOperand(),
                                     function_type->getParamType(1));
  auto *write_value = &I.getValueWritten();

  auto *llvm_call = builder.CreateCall(
      function_callee,
      llvm::ArrayRef({ assoc_value, assoc_index, write_value }));
  MEMOIR_NULL_CHECK(llvm_call, "Could not create the call for AssocWrite");

  auto *collection =
      builder.CreatePointerCast(llvm_call, I.getObjectOperand().getType());

  // Coalesce the result with the corresponding DefPHI.
  for (auto *user : I.getObjectOperand().users()) {
    auto *user_as_inst = dyn_cast<llvm::Instruction>(user);
    if (!user_as_inst) {
      continue;
    }
    if (!((user_as_inst->getParent() == I.getParent())
          && (user_as_inst > &I.getCallInst()))) {
      continue;
    }
    auto *user_as_memoir = MemOIRInst::get(*user_as_inst);
    if (!user_as_memoir) {
      continue;
    }

    if (auto *def_phi = dyn_cast<DefPHIInst>(user_as_memoir)) {
      this->def_phi_replacements[&def_phi->getCallInst()] = collection;
      break;
    }
  }

  this->markForCleanup(I);

#endif
  return;
}

void SSADestructionVisitor::visitAssocHasInst(AssocHasInst &I) {
#if USE_VECTOR
  auto &assoc_type =
      MEMOIR_SANITIZE(dyn_cast_or_null<AssocArrayType>(
                          TypeAnalysis::analyze(I.getObjectOperand())),
                      "Couldn't determine type of has collection");

  auto &key_type = assoc_type.getKeyType();
  auto &value_type = assoc_type.getValueType();

  auto key_code = key_type.get_code();
  auto value_code = value_type.get_code();
  auto name = *key_code + "_" + *value_code + "_hashtable__has";

  auto &M = MEMOIR_SANITIZE(I.getModule(), "Couldn't get module");
  auto *function = M.getFunction(name);
  auto function_callee = FunctionCallee(function);
  if (function == nullptr) {
    warnln("Couldn't find AssocHas for ", name);
    return;
  }

  MemOIRBuilder builder(I);

  auto *function_type = function_callee.getFunctionType();
  auto *assoc_value = builder.CreatePointerCast(&I.getObjectOperand(),
                                                function_type->getParamType(0));
  auto *assoc_key =
      builder.CreateBitOrPointerCast(&I.getKeyOperand(),
                                     function_type->getParamType(1));

  auto *llvm_call =
      builder.CreateCall(function_callee,
                         llvm::ArrayRef({ assoc_value, assoc_key }));
  MEMOIR_NULL_CHECK(llvm_call, "Could not create the call for AssocHas");

  I.getCallInst().replaceAllUsesWith(llvm_call);
  this->markForCleanup(I);
#endif
  return;
}

void SSADestructionVisitor::visitAssocRemoveInst(AssocRemoveInst &I) {
#if USE_VECTOR
  auto &assoc_type =
      MEMOIR_SANITIZE(dyn_cast_or_null<AssocArrayType>(
                          TypeAnalysis::analyze(I.getCollectionOperand())),
                      "Couldn't determine type of written collection");

  auto &key_type = assoc_type.getKeyType();
  auto &value_type = assoc_type.getValueType();

  auto key_code = key_type.get_code();
  auto value_code = value_type.get_code();
  auto name = *key_code + "_" + *value_code + "_hashtable__remove";

  auto &M = MEMOIR_SANITIZE(I.getModule(), "Couldn't get module");
  auto *function = M.getFunction(name);
  auto function_callee = FunctionCallee(function);
  if (function == nullptr) {
    warnln("Couldn't find AssocRemove name for ", name);
    return;
  }

  MemOIRBuilder builder(I);

  auto *function_type = function_callee.getFunctionType();
  auto *assoc = builder.CreatePointerCast(&I.getCollectionOperand(),
                                          function_type->getParamType(0));
  auto *assoc_key =
      builder.CreateBitOrPointerCast(&I.getKeyOperand(),
                                     function_type->getParamType(1));

  auto *llvm_call =
      builder.CreateCall(function_callee, llvm::ArrayRef({ assoc, assoc_key }));
  MEMOIR_NULL_CHECK(llvm_call, "Could not create the call for AssocRemove");

  auto *collection =
      builder.CreatePointerCast(llvm_call, I.getCollectionOperand().getType());

  // Coalesce the result with the corresponding DefPHI.
  for (auto *user : I.getCollectionOperand().users()) {
    auto *user_as_inst = dyn_cast<llvm::Instruction>(user);
    if (!user_as_inst) {
      continue;
    }
    if (!((user_as_inst->getParent() == I.getParent())
          && (user_as_inst > &I.getCallInst()))) {
      continue;
    }
    auto *user_as_memoir = MemOIRInst::get(*user_as_inst);
    if (!user_as_memoir) {
      continue;
    }

    if (auto *def_phi = dyn_cast<DefPHIInst>(user_as_memoir)) {
      this->def_phi_replacements[&def_phi->getCallInst()] = collection;
      break;
    }
  }

  this->markForCleanup(I);

#endif
  return;
}

void SSADestructionVisitor::visitUsePHIInst(UsePHIInst &I) {
  auto &used_collection = I.getUsedCollectionOperand();
  auto &collection = I.getCollectionValue();

  this->coalesce(collection, used_collection);

  this->markForCleanup(I);

  return;
}

void SSADestructionVisitor::visitDefPHIInst(DefPHIInst &I) {
  auto &defined_collection = I.getDefinedCollectionOperand();
  auto &collection = I.getCollectionValue();

  auto found_replacement = this->def_phi_replacements.find(&I.getCallInst());
  if (found_replacement != this->def_phi_replacements.end()) {
    this->coalesce(collection, *found_replacement->second);
  } else {
    this->coalesce(collection, defined_collection);
  }

  this->markForCleanup(I);

  return;
}

static void slice_to_view(SliceInst &I) {
  auto &call_inst = I.getCallInst();
  auto *view_func = FunctionNames::get_memoir_function(*call_inst.getModule(),
                                                       MemOIR_Func::VIEW);
  MEMOIR_NULL_CHECK(view_func, "Could not find the memoir view function");
  call_inst.setCalledFunction(view_func);
  return;
}

void SSADestructionVisitor::visitSliceInst(SliceInst &I) {
  auto &collection = I.getCollectionOperand();
  auto &slice = I.getSliceAsValue();

  // If the collection is dead immediately following,
  // then we can replace this slice with a view.
  if (!this->LA.is_live(collection, I)) {
    slice_to_view(I);
    return;
  }

  // If the slice is disjoint from the live slice range of the collection.
  bool is_disjoint = true;
  auto &slice_begin = I.getBeginIndex();
  auto &slice_end = I.getEndIndex();
  set<SliceInst *> slice_users = {};
  for (auto *user : collection.users()) {
    auto *user_as_inst = dyn_cast<llvm::Instruction>(user);
    if (!user_as_inst) {
      // This is an overly conservative check.
      is_disjoint = false;
      break;
    }

    // Check that the user is dominated by this instruction.
    if (!DT.dominates(&I.getCallInst(), user_as_inst)) {
      break;
    }

    auto *user_as_memoir = MemOIRInst::get(*user_as_inst);
    if (!user_as_memoir) {
      // Also overly conservative, we _can_ handle PHIs.
      is_disjoint = false;
      break;
    }

    if (auto *user_as_slice = dyn_cast<SliceInst>(user_as_memoir)) {
      // We will check all slice users for non-overlapping index spaces, _if_
      // there are no other users.
      slice_users.insert(user_as_slice);
      continue;
    }

    if (auto *user_as_access = dyn_cast<AccessInst>(user_as_memoir)) {
      // TODO: check the interval range of the index.
    }

    is_disjoint = false;
    break;
  }

  if (!is_disjoint) {
    return;
  }

  slice_users.erase(&I);

  // Check the slice users to see if they are non-overlapping.
  if (!slice_users.empty()) {
    auto &slice_begin = I.getBeginIndex();
    auto &slice_end = I.getEndIndex();
    set<SliceInst *> visited = {};
    list<llvm::Value *> limits = { &slice_begin, &slice_end };
    debugln("check non-overlapping");
    while (visited.size() < slice_users.size()) {
      bool found_new_limit = false;
      for (auto *user_as_slice : slice_users) {
        if (visited.find(user_as_slice) != visited.end()) {
          continue;
        }

        auto &user_slice_begin = user_as_slice->getBeginIndex();
        auto &user_slice_end = user_as_slice->getEndIndex();

        debugln("lower limit: ", *limits.front());
        debugln("upper limit: ", *limits.back());
        debugln(" user begin: ", user_slice_begin);
        debugln(" user   end: ", user_slice_end);

        // Check if this slice range is overlapping.
        if (&user_slice_begin == limits.back()) {
          visited.insert(user_as_slice);
          limits.push_back(&user_slice_end);
          found_new_limit = true;
        } else if (&user_slice_end == limits.front()) {
          visited.insert(user_as_slice);
          limits.push_front(&user_slice_begin);
          found_new_limit = true;
        }
      }

      // If we found a new limit, continue working.
      if (!found_new_limit) {
        break;
      }
    }

    // Otherwise, we need to bring out the big guns and check for relations.
    auto *slice_begin_expr = this->VN.get(slice_begin);
    auto *slice_end_expr = this->VN.get(slice_end);
    ValueExpression *new_lower_limit = nullptr;
    ValueExpression *new_upper_limit = nullptr;
    for (auto *user_as_slice : slice_users) {
      if (visited.find(user_as_slice) != visited.end()) {
        continue;
      }

      // Check if this slice range is non-overlapping, with an offset.
      auto *user_slice_begin_expr =
          this->VN.get(user_as_slice->getBeginIndex());
      MEMOIR_NULL_CHECK(user_slice_begin_expr,
                        "Error making value expression for begin index");
      auto *user_slice_end_expr = this->VN.get(user_as_slice->getEndIndex());
      MEMOIR_NULL_CHECK(user_slice_end_expr,
                        "Error making value expression for end index");

      debugln("Checking left");
      debugln("  ", user_as_slice->getBeginIndex());
      debugln("  ", slice_begin);
      auto check_left = *user_slice_end_expr < *slice_begin_expr;

      debugln("Checking right");
      debugln("  ", user_as_slice->getEndIndex());
      debugln("  ", slice_end);
      auto check_right = *user_slice_begin_expr > *slice_end_expr;

      if (check_left || check_right) {
        continue;
      } else {
        warnln("Big guns failed,"
               " open an issue if this shouldn't have happened.");
        is_disjoint = false;
      }
    }
  }

  if (is_disjoint) {
    slice_to_view(I);
  }

  return;
}

void SSADestructionVisitor::visitJoinInst(JoinInst &I) {
  auto &collection = I.getCollectionAsValue();
  auto num_joined = I.getNumberOfJoins();

  // For each join operand, if it is dead after the join, we can coallesce this
  // join with it.
  bool all_dead = true;
  for (auto join_idx = 0; join_idx < num_joined; join_idx++) {
    auto &joined_use = I.getJoinedOperandAsUse(join_idx);
    auto &joined_collection =
        MEMOIR_SANITIZE(joined_use.get(), "Use by join is NULL!");
    if (this->LA.is_live(joined_collection, I)) {
      all_dead = false;

      debugln("Live after join!");
      debugln("        ", joined_collection);
      debugln("  after ", I);

      break;
    }
  }

  if (!all_dead) {
    infoln("Not all incoming values are dead after a join.");
    infoln(" |-> ", I);
    return;
  }

  // If all operands of the join are views of the same collection:
  //  - If views are in order, coallesce resultant and the viewed collection.
  //  - Otherwise, determine if the size is the same after the join:
  //     - If the size is the same, convert to a swap.
  //     - Otherwise, convert to a remove.
  vector<size_t> view_indices = {};
  view_indices.reserve(num_joined);
  vector<ViewInst *> views = {};
  views.reserve(num_joined);
  bool all_views = true;
  llvm::Value *base_collection = nullptr;
  for (auto join_idx = 0; join_idx < num_joined; join_idx++) {
    auto &joined_use = I.getJoinedOperandAsUse(join_idx);
    auto &joined_collection =
        MEMOIR_SANITIZE(joined_use.get(), "Use by join is NULL!");

    auto *joined_as_inst = dyn_cast<llvm::Instruction>(&joined_collection);
    if (!joined_as_inst) {
      all_views = false;
      break;
    }

    auto *joined_collection_as_memoir = MemOIRInst::get(*joined_as_inst);
    llvm::Value *used_collection;
    if (auto *view_inst =
            dyn_cast_or_null<ViewInst>(joined_collection_as_memoir)) {
      auto &viewed_collection = view_inst->getCollectionOperand();
      used_collection = &viewed_collection;
      views.push_back(view_inst);
      view_indices.push_back(join_idx);
    } else {
      infoln("Join uses non-view");
      infoln(" |-> ", I);
      all_views = false;
      continue;
    }

    // See if we are still using the same base collection.
    if (base_collection == nullptr) {
      base_collection = used_collection;
      continue;
    } else if (base_collection == used_collection) {
      continue;
    } else {
      base_collection = nullptr;
      break;
    }
  }

  if (base_collection != nullptr && all_views) {
    infoln("Join uses a single base collection");
    infoln(" |-> ", I);

    // Determine if the size is the same.
    set<ViewInst *> visited = {};
    list<size_t> view_order = {};
    list<pair<llvm::Value *, llvm::Value *>> ranges_to_remove = {};
    llvm::Value *lower_limit = nullptr;
    llvm::Value *upper_limit = nullptr;
    bool swappable = true;
    while (visited.size() != views.size()) {
      bool new_limit = false;
      for (auto view_idx = 0; view_idx < views.size(); view_idx++) {
        auto *view = views[view_idx];
        auto &begin_index = view->getBeginIndex();
        auto &end_index = view->getEndIndex();
        if (lower_limit == nullptr) {
          lower_limit = &begin_index;
          upper_limit = &end_index;
          visited.insert(view);
          view_order.push_back(view_idx);
          new_limit = true;
          continue;
        } else if (lower_limit == &end_index) {
          lower_limit = &begin_index;
          visited.insert(view);
          view_order.push_front(view_idx);
          new_limit = true;
          continue;
        } else if (upper_limit == &begin_index) {
          upper_limit = &end_index;
          visited.insert(view);
          view_order.push_back(view_idx);
          new_limit = true;
          continue;
        }
      }

      if (new_limit) {
        continue;
      }

      // Determine the ranges that must be removed.
      auto *lower_limit_expr = this->VN.get(*lower_limit);
      auto *upper_limit_expr = this->VN.get(*upper_limit);
      ValueExpression *new_lower_limit_expr = nullptr;
      ViewInst *lower_limit_view = nullptr;
      ValueExpression *new_upper_limit_expr = nullptr;
      ViewInst *upper_limit_view = nullptr;
      for (auto view_idx = 0; view_idx < views.size(); view_idx++) {
        auto *view = views[view_idx];
        if (visited.find(view) != visited.end()) {
          continue;
        }

        // Find the nearest
        auto &view_begin_expr =
            MEMOIR_SANITIZE(this->VN.get(view->getBeginIndex()),
                            "Error making value expression for begin index");
        auto &view_end_expr =
            MEMOIR_SANITIZE(this->VN.get(view->getEndIndex()),
                            "Error making value expression for end index");

        auto check_left = view_end_expr < *lower_limit_expr;
        auto check_right = view_begin_expr > *upper_limit_expr;

        if (check_left) {
          // Check if this is the new lower bound.
          if (new_lower_limit_expr == nullptr) {
            new_lower_limit_expr = &view_end_expr;
            lower_limit_view = view;
            new_limit = true;
          } else if (view_end_expr < *new_lower_limit_expr) {
            new_lower_limit_expr = &view_end_expr;
            lower_limit_view = view;
            new_limit = true;
          }
          continue;
        } else if (check_right) {
          // Check if this is the new upper bound.
          if (new_upper_limit_expr == nullptr) {
            new_upper_limit_expr = &view_begin_expr;
            upper_limit_view = view;
            new_limit = true;
          } else if (view_begin_expr > *new_upper_limit_expr) {
            new_upper_limit_expr = &view_begin_expr;
            upper_limit_view = view;
            new_limit = true;
          }
          continue;
        }
      }

      // If we found a new limit, update the lower and/or upper limit and mark
      // the range for removal.
      if (new_limit) {
        if (lower_limit_view != nullptr) {
          auto &new_lower_limit = lower_limit_view->getEndIndex();
          ranges_to_remove.push_front(make_pair(&new_lower_limit, lower_limit));
          lower_limit = &new_lower_limit;
        }

        if (upper_limit_view != nullptr) {
          auto &new_upper_limit = upper_limit_view->getBeginIndex();
          ranges_to_remove.push_back(make_pair(upper_limit, &new_upper_limit));
          upper_limit = &new_upper_limit;
        }

        // Keep on chugging.
        continue;
      }

      println("!!!!");
      println("The join: ", I);
      MEMOIR_UNREACHABLE(
          "We shouldn't be here, couldnt' find out what to swap/remove forthe above join!");
    }

    if (swappable) {
      infoln("Convert the join to a swap.");
      infoln(" |-> ", I);
      infoln(" |-> Original order:");
      for (auto view_idx : view_order) {
        infoln(" | |-> ", *views[view_idx]);
      }
      infoln(" |-> New order:");
      for (auto view : views) {
        infoln(" | |-> ", *view);
      }

      auto new_idx = 0;
      auto i = 0;
      for (auto it = view_order.begin(); it != view_order.end(); ++it, i++) {
        auto orig_idx = *it;
        if (orig_idx != new_idx) {
          infoln(" |-> Swap ", orig_idx, " <=> ", new_idx);

          // Check that these views are the same size.
          auto &from_begin = views[orig_idx]->getBeginIndex();
          auto *from_begin_expr = this->VN.get(from_begin);
          auto &from_end = views[orig_idx]->getEndIndex();
          auto *from_end_expr = this->VN.get(from_end);
          auto &to_begin = views[new_idx]->getBeginIndex();
          auto *to_begin_expr = this->VN.get(to_begin);
          auto &to_end = views[new_idx]->getEndIndex();
          auto *to_end_expr = this->VN.get(to_end);

          // Create the expression for the size calculation.
          auto *from_size_expr = new BasicExpression(
              llvm::Instruction::Sub,
              vector<ValueExpression *>({ from_end_expr, from_begin_expr }));
          auto *to_size_expr = new BasicExpression(
              llvm::Instruction::Sub,
              vector<ValueExpression *>({ to_end_expr, to_begin_expr }));

          // If they are the same size, swap them.
          if (*from_size_expr == *to_size_expr) {
            // Create the swap.
            MemOIRBuilder builder(I);
            builder.CreateSeqSwapInst(base_collection,
                                      &from_begin,
                                      &from_end,
                                      &to_begin);

            // Swap the operands.
            auto &from_use = I.getJoinedOperandAsUse(orig_idx);
            auto &to_use = I.getJoinedOperandAsUse(new_idx);
            from_use.swap(to_use);

            // NOTE: we don't need to update other users here, as these views
            // were already checked to be dead following.

            // Update the view order post-swap.
            auto r = view_order.size() - 1;
            for (auto rit = view_order.rbegin(); r > i; ++rit, r--) {
              if (*rit == new_idx) {
                std::swap(*(views.begin() + i), *(views.begin() + r));
                std::swap(*it, *rit);
                break;
              }
            }
          } else {
            MEMOIR_UNREACHABLE("Size of from and to for swap are not the same! "
                               "Someone must have changed the program.");
          }
        }
        new_idx++;
      }

      // Replace the join with the base collection that has been swapped.
      this->coalesce(I, *base_collection);
      this->markForCleanup(I);
    }

#if USE_VECTOR
    auto &collection_type =
        MEMOIR_SANITIZE(dyn_cast_or_null<CollectionType>(
                            TypeAnalysis::analyze(I.getCallInst())),
                        "Couldn't determine type of join");

    auto &element_type = collection_type.getElementType();

    // TODO: make this more extensible.
    auto element_code = element_type.get_code();
    auto vector_remove_range_name = *element_code + "_vector__remove";

    auto &M = MEMOIR_SANITIZE(I.getModule(), "Couldn't get module");
    auto *function = M.getFunction(vector_remove_range_name);
    auto function_callee = FunctionCallee(function);
    if (function == nullptr) {
      println("Couldn't find vector remove range for ",
              vector_remove_range_name);
      MEMOIR_UNREACHABLE("see above");
    }

    MemOIRBuilder builder(I);

    auto *function_type = function_callee.getFunctionType();
    auto *vector_value =
        builder.CreatePointerCast(base_collection,
                                  function_type->getParamType(0));
#endif

    // Now to remove any ranges that were marked.
    for (auto range : ranges_to_remove) {
      auto *begin = range.first;
      auto *end = range.second;
      MemOIRBuilder builder(I);
      // TODO: check if this range is alive as a view, if it is we need to
      // convert it to a slice.
#if USE_VECTOR
      vector_value =
          builder.CreateCall(function_callee,
                             llvm::ArrayRef({ vector_value, begin, end }));
      MEMOIR_NULL_CHECK(vector_value,
                        "Could not create the call for vector remove range");
#else
      builder.CreateSeqRemoveInst(base_collection, begin, end);
#endif
    }

#if USE_VECTOR
    auto *collection =
        builder.CreatePointerCast(vector_value, I.getCallInst().getType());
    this->coalesce(I, *collection);
#endif

  } else if (base_collection != nullptr) /* not all_views */ {
    // Determine if the views are in order.
    // If they are, we do an insert.
    llvm::Value *lower_limit, *upper_limit;
    bool base_is_ordered = true;
    auto idx_it = view_indices.begin();
    for (auto view_it = views.begin(); view_it != views.end(); ++view_it) {
      auto *view = *view_it;
      auto cur_idx = *idx_it;
      if (++idx_it == view_indices.end()) {
        break;
      }
      auto next_idx = *idx_it;
      if (next_idx > cur_idx + 1) {
        auto *next_view = *(++view_it);
        --view_it; // revert the last iteration.

        auto &cur_view_end = view->getEndIndex();
        auto &next_view_begin = next_view->getBeginIndex();

        // If the views at this insertion point are the same, then we need to
        // insert.
        if (&cur_view_end == &next_view_begin) {
          MemOIRBuilder builder(I);

#if USE_VECTOR
          auto &collection_type =
              MEMOIR_SANITIZE(dyn_cast_or_null<CollectionType>(
                                  TypeAnalysis::analyze(I.getCallInst())),
                              "Couldn't determine type of join");

          auto &element_type = collection_type.getElementType();

          auto element_code = element_type.get_code();
          auto vector_remove_range_name = *element_code + "_vector__insert";

          auto &M = MEMOIR_SANITIZE(I.getModule(), "Couldn't get module");
          auto *function = M.getFunction(vector_remove_range_name);
          if (function == nullptr) {
            println("Couldn't find vector insert for ",
                    vector_remove_range_name);
            MEMOIR_UNREACHABLE("see above");
          }
          auto function_callee = FunctionCallee(function);

          auto *function_type = function_callee.getFunctionType();
          auto *vector =
              builder.CreatePointerCast(base_collection,
                                        function_type->getParamType(0));
#endif

          llvm::Value *insertion_point = nullptr;
          llvm::Value *last_inserted_collection = nullptr;
          for (auto idx = cur_idx + 1; idx < next_idx; idx++) {
            // Get the collection to insert.
            auto &collection_to_insert = I.getJoinedOperand(idx);

            // Track the insertion point.
            if (insertion_point == nullptr) {
              insertion_point = &cur_view_end;
            } else {
              // TODO: improve me.
#if USE_VECTOR
              auto vector_size_name = *element_code + "_vector__size";

              auto *size_function = M.getFunction(vector_size_name);
              if (size_function == nullptr) {
                println("Couldn't find vector size for ", vector_size_name);
                MEMOIR_UNREACHABLE("see above");
              }
              auto size_function_callee = FunctionCallee(size_function);

              auto *last_size = builder.CreateCall(
                  size_function_callee,
                  llvm::ArrayRef({ last_inserted_collection }));
#else
              auto *last_size =
                  &builder.CreateSizeInst(last_inserted_collection)
                       ->getCallInst();
#endif
              insertion_point = builder.CreateAdd(insertion_point, last_size);
            }

            // TODO: See if the collection is a single element, if so use its
            // specialized insert method. Tricky part is finding the value to
            // insert.
            // auto &inserted_collection = I.getJoinedCollection(idx);
            // if (auto *inserted_alloc =
            //         dyn_cast<BaseCollection>(inserted_collection)) {
            //   auto *collection_alloc = inserted_alloc->getAllocation();
            //   auto *sequence_alloc =
            //       dyn_cast_or_null<SequenceAllocInst>(collection_alloc);
            //   MEMOIR_NULL_CHECK(sequence_alloc,
            //                     "Allocation being inserted is not a
            //                     sequence!");
            //   auto &sequence_size = sequence_alloc->getSizeOperand();
            //   if (auto *size_as_const_int =
            //           dyn_cast<llvm::ConstantInt *>(&sequence_size)) {
            //     if (size_as_const_int->isOne()) {
            //       auto *element_type = sequence_alloc->getElementType();
            //       builder.CreateSeqInsertInst(base_collection,
            //       insertion_point, value_to_insert
            //     }
            //   }
            // }

            // Create the insert instruction.
#if USE_VECTOR
            auto *vector_to_insert =
                builder.CreatePointerCast(&collection_to_insert,
                                          function_type->getParamType(2));
            vector = builder.CreateCall(
                function_callee,
                llvm::ArrayRef({ vector, insertion_point, vector_to_insert }));
            last_inserted_collection = vector;
#else
            builder.CreateSeqInsertSeqInst(base_collection,
                                           insertion_point,
                                           &collection_to_insert);
            last_inserted_collection = &collection_to_insert;
#endif
          }

#if USE_VECTOR
          vector = builder.CreatePointerCast(vector, I.getCallInst().getType());
          this->coalesce(I, *vector);
#else
          this->coalesce(I, *base_collection);
#endif
          this->markForCleanup(I);
        }
      }
    }
  }

  // Otherwise, some operands are views from a different collection:
  //  - If the size is the same, convert to a swap.
  //  - Otherwise, convert to an append.
  else /* not base_collection, not all_views*/ {

    // If there are no views being used, we simply have an append.
    if (views.empty()) {
      MemOIRBuilder builder(I);

      auto &first_joined_operand = I.getJoinedOperand(0);

#if USE_VECTOR
      auto &collection_type =
          MEMOIR_SANITIZE(dyn_cast_or_null<CollectionType>(
                              TypeAnalysis::analyze(I.getCallInst())),
                          "Couldn't determine type of join");
      auto &element_type = collection_type.getElementType();

      auto element_code = element_type.get_code();

      auto insert_name = *element_code + "_vector__insert";
      auto size_name = *element_code + "_vector__size";

      auto &M = MEMOIR_SANITIZE(I.getModule(), "Couldn't get module");

      auto *insert_function = M.getFunction(insert_name);
      if (insert_function == nullptr) {
        println("Couldn't find vector insert for ", insert_name);
        MEMOIR_UNREACHABLE("see above");
      }
      auto insert_function_callee = FunctionCallee(insert_function);
      auto *insert_function_type = insert_function_callee.getFunctionType();

      auto *size_function = M.getFunction(size_name);
      if (size_function == nullptr) {
        println("Couldn't find vector size for ", size_name);
        MEMOIR_UNREACHABLE("see above");
      }
      auto size_function_callee = FunctionCallee(size_function);
      auto *size_function_type = size_function_callee.getFunctionType();

      auto *vector =
          builder.CreatePointerCast(&first_joined_operand,
                                    insert_function_type->getParamType(0));

      auto *insertion_point =
          builder.CreateCall(size_function_callee, llvm::ArrayRef({ vector }));
#endif

      for (auto join_idx = 1; join_idx < num_joined; join_idx++) {
        auto &joined_operand = I.getJoinedOperand(join_idx);
#if USE_VECTOR
        auto *insertion_point = builder.CreateCall(size_function_callee,
                                                   llvm::ArrayRef({ vector }));

        auto *vector_to_insert =
            builder.CreatePointerCast(&joined_operand,
                                      insert_function_type->getParamType(2));
        vector = builder.CreateCall(
            insert_function_callee,
            llvm::ArrayRef<llvm::Value *>(
                { vector, insertion_point, vector_to_insert }));
#else
        builder.CreateSeqAppendInst(&first_joined_operand, &joined_operand);
#endif
      }

      this->coalesce(I, first_joined_operand);
      this->markForCleanup(I);
    }
  }

  return;
} // namespace llvm::memoir

void SSADestructionVisitor::cleanup() {
  for (auto *inst : instructions_to_delete) {
    infoln(*inst);
    inst->eraseFromParent();
  }
}

void SSADestructionVisitor::coalesce(MemOIRInst &I, llvm::Value &replacement) {
  this->coalesce(I.getCallInst(), replacement);
}

void SSADestructionVisitor::coalesce(llvm::Value &V, llvm::Value &replacement) {
  infoln("Coalesce:");
  infoln("  ", V);
  infoln("  ", replacement);
  this->coalesced_values[&V] = &replacement;
}

llvm::Value *SSADestructionVisitor::find_replacement(llvm::Value *value) {
  auto *replacement_value = value;
  auto found = this->replaced_values.find(value);
  while (found != this->replaced_values.end()) {
    replacement_value = found->second;
    found = this->replaced_values.find(replacement_value);
  }
  return replacement_value;
}

void SSADestructionVisitor::do_coalesce(llvm::Value &V) {
  auto found_coalesce = this->coalesced_values.find(&V);
  if (found_coalesce == this->coalesced_values.end()) {
    return;
  }

  auto *replacement = this->find_replacement(found_coalesce->second);

  infoln("Coalescing:");
  infoln("  ", V);
  infoln("  ", *replacement);

  V.replaceAllUsesWith(replacement);

  // Update the types of users.
  // for (auto *user : V.users()) {
  //   if (auto *user_as_phi = dyn_cast<llvm::PHINode>(user)) {
  //     if (user_as_phi->getType() != V.getType()) {
  //       user_as_phi->mutateType(V.getType());
  //     }
  //   }
  // }

  this->replaced_values[&V] = replacement;
}

void SSADestructionVisitor::markForCleanup(MemOIRInst &I) {
  this->markForCleanup(I.getCallInst());
}

void SSADestructionVisitor::markForCleanup(llvm::Instruction &I) {
  this->instructions_to_delete.insert(&I);
}

} // namespace llvm::memoir
