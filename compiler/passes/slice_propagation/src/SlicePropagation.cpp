// LLVM
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

// MemOIR
#include "memoir/ir/Builder.hpp"

// Slice Propagation
#include "SlicePropagation.hpp"

namespace llvm::memoir {

// Constructor.
SlicePropagation::SlicePropagation(llvm::Module &M,
                                   llvm::Pass &P,
                                   llvm::noelle::Noelle &noelle)
  : M(M),
    P(P),
    noelle(noelle),
    TA(TypeAnalysis::get()),
    SA(StructAnalysis::get()),
    CA(CollectionAnalysis::get(noelle)) {
  // Do nothing.
}

// Destructor
SlicePropagation::~SlicePropagation() {
  // Do nothing.
}

// Top-level analysis.
bool SlicePropagation::analyze() {
  println();
  println("=====================================");
  println("SlicePropagation: performing analysis");
  println();

  // auto loops = noelle.getLoops();
  // MEMOIR_NULL_CHECK(loops, "Unable to get the loops from NOELLE!");

  // Gather all memoir slices from each LLVM Function.
  map<llvm::Function *, set<SliceInst *>> slice_instructions = {};
  for (auto &F : M) {
    for (auto &BB : F) {
      for (auto &I : BB) {
        if (auto memoir_inst = MemOIRInst::get(I)) {
          if (auto slice_inst = dyn_cast<SliceInst>(memoir_inst)) {
            slice_instructions[&F].insert(slice_inst);
          }
        }
      }
    }
  }

  // Perform a backwards flow analysis to slice the collection as early as
  // possible.
  auto VN = new ValueNumbering(this->M);
  visited.clear();
  for (auto const &[func, slice_insts] : slice_instructions) {
    for (auto slice_inst : slice_insts) {
      println();
      println("Inspecting slice: ", *slice_inst);

      // Get the LLVM representation of this slice instruction.
      auto &llvm_slice_inst = slice_inst->getCallInst();

      // Check if this slice instruction has already been visited.
      if (visited.find(&llvm_slice_inst) != visited.end()) {
        println("  slice has already been visited.");
        continue;
      }

      // Check that the sliced operand is only used by this slice instruction.
      // TODO: extend this to allow for multiple slices to be propagated.
      bool slice_is_only_user = true;
      auto &sliced_operand = slice_inst->getCollectionOperand();
      for (auto &use : sliced_operand.uses()) {
        auto *user = use.getUser();

        // If the user is the slice inst.
        if (user == &llvm_slice_inst) {
          continue;
        }

        // If the user is not a real one. (i.e. a delete inst).
        if (auto *user_as_inst = dyn_cast<llvm::Instruction>(user)) {
          if (auto *user_as_memoir = MemOIRInst::get(*user_as_inst)) {
            if (isa<DeleteCollectionInst>(user_as_memoir)) {
              continue;
            }
          }
        }

        // Otherwise, there is another user of the slice.
        slice_is_only_user = false;
        break;
      }

      if (!slice_is_only_user) {
        println("Slice is not the only user.");
        continue;
      }

      // TODO
      // Determine the left and right index expressions.
      auto left_index_expr = VN->get(slice_inst->getBeginIndex());
      println(*left_index_expr);
      auto right_index_expr = VN->get(slice_inst->getEndIndex());
      println(*right_index_expr);

      // Initialize the state.
      this->candidate =
          new SlicePropagationCandidate(slice_inst->getCollectionOperandAsUse(),
                                        left_index_expr,
                                        right_index_expr);
      this->all_candidates.insert(this->candidate);

      // Look at the sliced collection to see if it can be sliced.
      auto &sliced_collection = slice_inst->getCollection();
      this->visitCollection(sliced_collection);
    }
  }

  println();
  println("SlicePropagation: done analyzing");
  println("================================");
  println();

  println();
  println("==================================================");
  println("SlicePropagation: BEGIN printing slice candidates.");
  println();

  for (auto leaf : this->leaf_candidates) {
    println("Found leaf candidate:");
    println("  ", *leaf->use.get());
    println("  ", *leaf->left_index);
    println("  ", *leaf->right_index);
    println();
  }

  println();
  println("SlicePropagation: DONE printing slice candidates.");
  println("=================================================");
  println();

  return true;
}

// Helper macros.
#define CHECK_VISITED(V)                                                       \
  if (this->checkVisited(V)) {                                                 \
    return false;                                                              \
  }                                                                            \
  this->recordVisit(V);

// Visitor methods.
bool SlicePropagation::visitBaseCollection(BaseCollection &C) {
  // If the sliced collection is the result of an allocation, slice the
  // allocation itself.
  println("Visiting a base collection.");
  auto &alloc_inst = C.getAllocation();
  auto &llvm_alloc_inst = alloc_inst.getCallInst();

  // Check if we have already visited this instruction.
  CHECK_VISITED(llvm_alloc_inst);

  // Double check that this is a sequence allocation instruction.
  auto sequence_alloc_inst = dyn_cast<SequenceAllocInst>(&alloc_inst);
  if (!sequence_alloc_inst) {
    println("  Sliced allocation is not a sequence!");
    return false;
  }

  // Get the candidate information.
  auto &parent_candidate =
      sanitize(this->candidate, "Parent candidate is NULL");
  auto *left_index = parent_candidate.left_index;
  auto *right_index = parent_candidate.right_index;

  // Check that this collection's only user is the parent candidate.
  if (!llvm_alloc_inst.hasOneUse()) {
    println("  Sliced allocation has more than one use!");
    return false;
  }

  // Ensure that the slice index is _definitely_ larger than the allocation
  // size.
  auto *left_as_const = as<llvm::ConstantInt>(left_index);
  if (!left_as_const) {
    println("  Left index is not a constant integer");
    return false;
  }
  auto left_as_int = left_as_const->getSExtValue();
  if (left_as_int != 0) {
    println("  Left index is not a constant zero");
    return false;
  }

  auto *right_as_const = as<llvm::ConstantInt>(right_index);
  if (!right_as_const) {
    println("  Right index is not a constant integer");
    return false;
  }

  // Compare the size of this allocation with the right index value.
  auto &size_value = sequence_alloc_inst->getSizeOperand();
  auto size_as_const = dyn_cast<llvm::ConstantInt>(&size_value);
  if (!size_as_const) {
    println("  Size of sequence allocation is not constant");
    return false;
  }

  auto size_as_int = size_as_const->getZExtValue();
  auto right_as_int = right_as_const->getSExtValue();
  // Handle size relative slices.
  if (right_as_int < 0) {
    right_as_int = size_as_int + right_as_int;
  }
  if (size_as_int <= right_as_int) {
    println("  Size of sequence is less than the right index");
    return false;
  }

  // Mark the collection allocation for slicing.
  this->leaf_candidates.insert(this->candidate);

  return true;
}

bool SlicePropagation::visitFieldArray(FieldArray &C) {
  println("Visiting a field array.");

  return false;
}

bool SlicePropagation::visitNestedCollection(NestedCollection &C) {
  println("Visiting a nested collection.");

  // Check if we have already visited this instruction.
  CHECK_VISITED(C.getAccess().getCallInst());

  return false;
}

bool SlicePropagation::visitReferencedCollection(ReferencedCollection &C) {
  println("Visiting a referenced collection.");

  MEMOIR_UNREACHABLE("References to collections are deprecated!");

  return false;
}

bool SlicePropagation::visitControlPHICollection(ControlPHICollection &C) {
  println("Visiting a control PHI collection.");
  auto &llvm_phi = C.getPHI();

  println("  ", llvm_phi);

  // Check if we have already visited this instruction.
  CHECK_VISITED(llvm_phi);

  // Create the candidate for this control phi.
  auto parent_candidate = this->candidate;

  // Check that this collection's only user is the parent candidate.
  if (!llvm_phi.hasOneUse()) {
    println("  Control PHI has more than one use!");
    return false;
  }

  // Iterate through all incoming edges.
  bool marked = false;
  for (auto idx = 0; idx < C.getNumIncoming(); idx++) {
    auto &incoming = C.getIncomingCollection(idx);
    println("  incoming ", std::to_string(idx), ": ", incoming);

    // Build the new candidate.
    auto &incoming_use = llvm_phi.getOperandUse(idx);
    this->candidate =
        new SlicePropagationCandidate(incoming_use, parent_candidate);

    // Recurse.
    marked |= this->visitCollection(incoming);
  }

  return marked;
}

bool SlicePropagation::visitRetPHICollection(RetPHICollection &C) {
  println("Visiting a return PHI collection.");

  auto &llvm_call = C.getCall();

  // Check if we have already visited this instruction.
  CHECK_VISITED(llvm_call);

  // Save the parent candidate.
  auto parent_candidate = this->candidate;

  // Check that this collection's only user is the parent candidate.
  if (!llvm_call.hasOneUse()) {
    println("  Return PHI has more than one use!");
    return false;
  }

  // Iterate through all possible return values.
  // NOTE: This should be clean with noelle's normalization providing a single
  // return point for all functions.
  bool marked = false;
  for (auto ret_idx = 0; ret_idx < C.getNumIncoming(); ret_idx++) {
    auto &incoming = C.getIncomingCollection(ret_idx);
    println("  incoming ", std::to_string(ret_idx), ": ", incoming);

    // Build the candidate.
    auto &incoming_ret = C.getIncomingReturn(ret_idx);
    auto &incoming_use = incoming_ret.getOperandUse(0);
    this->candidate =
        new SlicePropagationCandidate(incoming_use, parent_candidate);
    this->candidate->call_context = &(C.getCall());

    // Recurse.
    marked |= this->visitCollection(incoming);
  }

  return marked;
}

bool SlicePropagation::visitArgPHICollection(ArgPHICollection &C) {
  println("Visiting an argument PHI collection.");

  auto &llvm_arg = C.getArgument();

  // Check if we have already visited this instruction.
  CHECK_VISITED(llvm_arg);

  // Create the candidate for this control phi.
  auto parent_candidate = this->candidate;

  // Check that this collection's only user is the parent candidate.
  if (!llvm_arg.hasOneUse()) {
    println("  Argument has more than one use!");
    return false;
  }

  // Iterate through all incoming arguments.
  bool marked = false;
  for (auto idx = 0; idx < C.getNumIncoming(); idx++) {
    auto &incoming = C.getIncomingCollection(idx);
    println("  incoming ", std::to_string(idx), ": ", incoming);

    // Build the candidate.
    auto &incoming_call = C.getIncomingCall(idx);
    auto &incoming_arg = incoming_call.getArgOperandUse(llvm_arg.getArgNo());
    this->candidate =
        new SlicePropagationCandidate(incoming_arg, parent_candidate);

    // Recurse.
    marked |= this->visitCollection(incoming);
  }

  return marked;
}

bool SlicePropagation::visitDefPHICollection(DefPHICollection &C) {
  println("Visiting a definition PHI collection.");

  // Check if we have already visited this instruction.
  CHECK_VISITED(C.getAccess().getCallInst());

  return false;
}

bool SlicePropagation::visitUsePHICollection(UsePHICollection &C) {
  println("Visiting a use PHI collection.");

  // Check if we have already visited this instruction.
  CHECK_VISITED(C.getAccess().getCallInst());

  return false;
}

bool SlicePropagation::visitJoinPHICollection(JoinPHICollection &C) {
  println("Visiting a join PHI collection.");

  // Get the MemOIR Inst and LLVM CallInst
  auto &join_inst = C.getJoin();
  auto &llvm_join_inst = join_inst.getCallInst();

  println("  ", join_inst);

  // FIXME: this is here as a hack to get around invariant-length sequences
  // being slice propagated.
  if (join_inst.getNumberOfJoins() == 2) {
    return false;
  }

  // Check if we have already visited this instruction.
  CHECK_VISITED(llvm_join_inst);

  // Get the candidate information.
  auto parent_candidate = this->candidate;

  // Check that this collection's only user is the parent candidate.
  if (!llvm_join_inst.hasOneUse()) {
    println("  Join Inst has more than one use!");
    return false;
  }

  // Get the slice information.
  auto left_index = parent_candidate->left_index;
  auto right_index = parent_candidate->right_index;

  // If the first index is a constant zero, recurse on the first
  // operand of the Join.
  // TODO: recurse on the last operand if the index is a constant -1.
  if (auto left_constant_int = as<llvm::ConstantInt>(left_index)) {
    if (left_constant_int->getZExtValue() == 0) {
      // Build the candidate.
      auto &first_operand_as_use = join_inst.getJoinedOperandAsUse(0);
      this->candidate =
          new SlicePropagationCandidate(first_operand_as_use, parent_candidate);

      // Recurse on the left-most operand.
      if (this->visitCollection(C.getJoinedCollection(0))) {
        return true;
      }
    }
  }

  // Mark the joined collection as being prepared for slicing.
  this->leaf_candidates.insert(parent_candidate);
  println("Adding leaf ", C);

  return true;
}

bool SlicePropagation::visitSliceCollection(SliceCollection &C) {
  println("Visiting a slice collection.");

  // Get the MemOIR Inst and the LLVM CallInst.
  auto &slice_inst = C.getSlice();
  auto &llvm_slice_inst = slice_inst.getCallInst();

  // Check if we have already visited this instruction.
  CHECK_VISITED(llvm_slice_inst);

  // Save the parent candidate.
  auto *parent_candidate = this->candidate;

  // Check that this collection's only user is the parent candidate.
  if (!llvm_slice_inst.hasOneUse()) {
    println("  Slice inst has more than one use!");
    return false;
  }

  // Get the slice range.
  auto *left_index = parent_candidate->left_index;
  auto *right_index = parent_candidate->right_index;

  // Mark the collection being sliced if the left index of this
  // SliceCollection is a constant 0.
  // TODO: extend the above case to handle masking the slice further and
  // applying it to the sliced collection.
  if (auto *left_index_as_constant_int = as<llvm::ConstantInt>(left_index)) {
    if (left_index_as_constant_int->getZExtValue() == 0) {
      // Check if the left and right indices of this slice are the same as the
      // slice candidate.
      auto &slice_left_index = slice_inst.getBeginIndex();
      auto *slice_left_index_as_constant_int =
          dyn_cast<llvm::ConstantInt>(&slice_left_index);
      if (!slice_left_index_as_constant_int) {
        println("Left index of slice inst is not a constant int");
        return false;
      }
      if (slice_left_index_as_constant_int->getSExtValue() != 0) {
        println("Left index of slice inst is not a constant zero");
        return false;
      }

      auto &slice_right_index = slice_inst.getEndIndex();
      if (*right_index != slice_right_index) {
        println("Right indices are not equivalent");
        return false;
      }

      // Build the candidate.
      this->candidate =
          new SlicePropagationCandidate(slice_inst.getCollectionOperandAsUse(),
                                        parent_candidate);

      // Recurse.
      if (this->visitCollection(C.getSlicedCollection())) {
        return true;
      }
    }
  }

  // Mark the joined collection as being prepared for slicing.
  // this->leaf_candidates.insert(parent_candidate);

  return false;
}

// Top-level transformation.
bool SlicePropagation::transform() {
  println();
  println("SlicePropagation: begin transform.");
  println();

  // For each slice candidate, attempt to propagate the slice to it.
  for (auto candidate : this->leaf_candidates) {
    // Perform sanity check.
    MEMOIR_NULL_CHECK(candidate, "Leaf candidate is NULL!");

    // Handle the candidate.
    this->handleCandidate(*candidate);
  }

  println();
  println("SlicePropagation: performing dead code elimination.");

  // Drop all references.
  set<llvm::Value *> values_ready_to_delete = {};
  for (auto dead_value : this->values_to_delete) {
    // Sanity check.
    if (!dead_value) {
      continue;
    }

    // If this is an instruction, let's drop all references first.
    if (auto dead_inst = dyn_cast<llvm::Instruction>(dead_value)) {
      dead_inst->removeFromParent();
      dead_inst->dropAllReferences();
    }

    values_ready_to_delete.insert(dead_value);
  }

  // Delete values.
  for (auto dead_value : values_ready_to_delete) {
    dead_value->deleteValue();
  }

  println();
  println("SlicePropagation: end transform.");
  println();

  return false;
}

// Transformation.
llvm::Value *SlicePropagation::handleCandidate(SlicePropagationCandidate &SPC) {
  // Visit this slice candidate.
  auto &candidate_use = SPC.use;
  auto candidate_user = candidate_use.getUser();
  MEMOIR_NULL_CHECK(candidate_user, "Candidate user is NULL!");
  auto candidate_value = candidate_use.get();

  println("Slicing");
  println("  collection: ", *candidate_value);
  println("        left: ", candidate->left_index);
  println("       right: ", candidate->right_index);

  llvm::Value *rebuild = nullptr;
  if (auto argument = dyn_cast<llvm::Argument>(candidate_value)) {
    // We have an LLVM argument.
    println("Visiting an argument");
    println(*argument);

    // Set the candidate.
    this->candidate = &SPC;

    // Attempt to rebuild the collection.
    rebuild = this->visitArgument(*argument);

  } else if (auto inst = dyn_cast<llvm::Instruction>(candidate_value)) {
    // We have an LLVM instruction.
    println("Visiting an instruction");
    println(*inst);

    // Set the candidate.
    this->candidate = &SPC;

    rebuild = this->visit(*inst);
  }

  // If rebuild was successful, propagate it along.
  if (rebuild) {
    auto parent_spc = &SPC;
    while (parent_spc) {
      // Get the Use information.
      auto *use = &(parent_spc->use);
      auto *user = use->getUser();
      auto *used_value = use->get();

      // Map the Use, if necessary.
      if (this->current_value_map) {
        ValueMapper mapper(*this->current_value_map);
        auto *mapped_user_as_value = mapper.mapValue(*user);
        auto *mapped_user = dyn_cast<llvm::User>(mapped_user_as_value);
        MEMOIR_NULL_CHECK(
            mapped_user,
            "Couldn't map the user, don't know how to propagate the slice");
        auto &mapped_use = mapped_user->getOperandUse(use->getOperandNo());

        println("original value: ", *used_value);
        println("  mapped value: ", *mapped_use.get());
        println(" original user: ", *user);
        println("   mapped user: ", *mapped_user);

        // Save the mapped state.
        use = &mapped_use;
        user = mapped_user;
        used_value = mapped_use.get();
      }

      println("Propagating rebuild to ", *user);

      // Sanity check.
      MEMOIR_NULL_CHECK(user, "Slice propagation candidate has NULL user");
      MEMOIR_NULL_CHECK(used_value,
                        "Slice propagation candidate has NULL used value");

      // Handle the user.
      if (auto user_as_inst = dyn_cast<llvm::Instruction>(user)) {
        println("Forward propagating to :");
        println("  ", *user_as_inst);

        // If this is a memoir instruction.
        if (auto user_as_memoir = MemOIRInst::get(*user_as_inst)) {
          if (auto user_as_slice = dyn_cast<SliceInst>(user_as_memoir)) {
            user_as_inst->replaceAllUsesWith(rebuild);
            this->markForDeletion(*user_as_inst);
          } else if (auto user_as_join = dyn_cast<JoinInst>(user_as_memoir)) {
            println(*user_as_join);
            user_as_inst->replaceAllUsesWith(rebuild);
            this->markForDeletion(*user_as_inst);
          }
        } else if (auto user_as_phi = dyn_cast<llvm::PHINode>(user_as_inst)) {
          println("original  user: ", *use->getUser());
          println("updated used value: ", *use->get());
          use->set(rebuild);
          println("updated user: ", *use->getUser());
          println("updated used value: ", *use->get());
          break;
        }
      }

      // Get the parent and continue.
      parent_spc = parent_spc->parent;
    }

    return nullptr;
  }

  // Otherwise, go to the parent slice candidate and try again.
  auto parent_slice = SPC.parent;
  if (parent_slice) {
    handleCandidate(*parent_slice);
  }

  return nullptr;
}

// InstVisitor methods for transformation.
llvm::Value *SlicePropagation::visitArgument(llvm::Argument &A) {
  return nullptr;
}

llvm::Value *SlicePropagation::visitInstruction(llvm::Instruction &I) {
  return nullptr;
}

llvm::Value *SlicePropagation::visitLLVMCallInst(llvm::CallInst &I) {
  return nullptr;
}

llvm::Value *SlicePropagation::visitPHINode(llvm::PHINode &I) {
  return nullptr;
}

llvm::Value *SlicePropagation::visitSequenceAllocInst(SequenceAllocInst &I) {
  // Get the information for how this instruction should be sliced.
  auto &llvm_inst = I.getCallInst();

  // Get the candidate.
  auto candidate = this->candidate;
  auto left_index = candidate->left_index;
  auto right_index = candidate->right_index;

  // Check that the left index is a constant zero.
  auto constant_int = as<llvm::ConstantInt>(left_index);
  // dyn_cast<llvm::ConstantInt>(left_index);
  if (!constant_int) {
    println("Slicing an allocation with a non-ConstantInt left index "
            "is unsupported at the moment");
    return nullptr;
  }

  if (constant_int->getZExtValue() != 0) {
    println("Slicing an allocation with the left index "
            "being non-zero is unsupported at the moment.");
    return nullptr;
  }

  // Get the size operand of the allocation instruction.
  auto &size_operand_as_use = I.getSizeOperandAsUse();

  // Check that the allocation and slice operand are both available in the
  // same function.
  // TODO: add function versioning to elide this issue.
  auto &alloc_bb =
      sanitize(llvm_inst.getParent(),
               "SequenceAllocInst is not attached to a basic block!");
  auto &alloc_func =
      sanitize(alloc_bb.getParent(),
               "SequenceAllocInst is not attached to a function!");

  // Check if the right index expression is available at this allocation
  // instruction.
  auto &DTA = this->P.getAnalysis<DominatorTreeWrapperPass>(alloc_func);
  auto const &DT = DTA.getDomTree();
  if (!right_index->isAvailable(llvm_inst, &DT)) {
    // If we got here, the right index wasn't available. Return NULL.
    println("SequenceAllocInst: right index is not available");
    return nullptr;
  }

  // Materialize the value.
  auto materialized_right_index =
      right_index->materialize(llvm_inst,
                               nullptr /* builder */,
                               &DT,
                               candidate->call_context);

  // If we weren't able to materialize the right index, return NULL.
  if (!materialized_right_index) {
    println("SequenceAllocInst: right index could not be materialized");
    return nullptr;
  }

  // Save the value mapper.
  auto *versioned_function = right_index->getVersionedFunction();
  auto *value_mapping = right_index->getValueMapping();
  auto *versioned_call = right_index->getVersionedCall();
  if (versioned_function && value_mapping) {
    this->function_versions[candidate->call_context] =
        make_pair(versioned_function, value_mapping);
    this->current_function_version = versioned_function;
    this->current_value_map = value_mapping;

    // Map the use.
    ValueMapper mapper(*value_mapping);
    auto *mapped_user_as_value =
        mapper.mapValue(*(size_operand_as_use.getUser()));
    auto *mapped_user = dyn_cast<llvm::User>(mapped_user_as_value);
    MEMOIR_NULL_CHECK(
        mapped_user,
        "Couldn't map the user, don't know how to propagate the slice");
    auto &mapped_use =
        mapped_user->getOperandUse(size_operand_as_use.getOperandNo());

    // If we were able to materialize it, set the size operand to be the
    // materialized value and return the LLVM Instruction.
    mapped_use.set(materialized_right_index);

    return mapped_user;
  }

  // Otherwise, update the use and return.
  size_operand_as_use.set(materialized_right_index);

  return &llvm_inst;
}

llvm::Value *SlicePropagation::visitJoinInst(JoinInst &I) {
  // Get the information for how this instruction should be sliced.
  auto &llvm_inst = I.getCallInst();
  auto left_index = this->candidate->left_index;
  auto right_index = this->candidate->right_index;

  println();
  println("Slicing ", I);
  println("   left = ", *left_index);
  println("  right = ", *right_index);

  // Check that the left index is a constant zero.
  auto constant_expr = dyn_cast<ConstantExpression>(left_index);
  if (!constant_expr) {
    println("Slicing a join with a non-constant left index "
            "is unsupported at the moment");
    return nullptr;
  }
  auto &constant = constant_expr->getConstant();
  auto constant_int = dyn_cast<ConstantInt>(&constant);
  if (!constant_int) {
    println("Slicing a join with a non-constant left index "
            " is unsupported at the moment");
    return nullptr;
  }
  if (constant_int->getZExtValue() != 0) {
    println("Slicing a join with the left index "
            "being non-zero is unsupported at the moment.");
    return nullptr;
  }

  // Get the function parent of this instruction.
  auto parent_bb = llvm_inst.getParent();
  if (!parent_bb) {
    println("JoinInst doesn't belong to a basic block");
    return nullptr;
  }
  auto parent_func = parent_bb->getParent();
  if (!parent_func) {
    println("JoinInst doesn't belong to a function");
    return nullptr;
  }

  // Ensure that the right index is available at this point.
  auto &DTA = this->P.getAnalysis<DominatorTreeWrapperPass>(*parent_func);
  auto const &DT = DTA.getDomTree();
  if (!right_index->isAvailable(llvm_inst,
                                &DT,
                                this->candidate->call_context)) {
    println("Right index is not available!");
    return nullptr;
  }

  // Materialize the right index.
  auto *materialized_right_index =
      right_index->materialize(llvm_inst,
                               nullptr,
                               &DT,
                               this->candidate->call_context);
  MEMOIR_NULL_CHECK(materialized_right_index,
                    "Couldn't materialize the right index");

  auto *versioned_function = right_index->getVersionedFunction();
  auto *value_mapping = right_index->getValueMapping();
  JoinInst *join_inst = &I;
  if (versioned_function && value_mapping) {
    println("join had a versioning");
    this->function_versions[candidate->call_context] =
        make_pair(versioned_function, value_mapping);
    this->current_function_version = versioned_function;
    this->current_value_map = value_mapping;

    // Construct a value mapper.
    ValueMapper mapper(*right_index->getValueMapping());

    // Map this instruction.
    auto *mapped_value = mapper.mapValue(llvm_inst);
    auto *mapped_inst = dyn_cast<llvm::Instruction>(mapped_value);
    MEMOIR_NULL_CHECK(mapped_inst,
                      "Couldn't map the instruction to an instruction");

    auto mapped_memoir_inst = MemOIRInst::get(*mapped_inst);
    MEMOIR_NULL_CHECK(
        mapped_memoir_inst,
        "JoinInst being visited is no longer a MemOIRInst after versioning");
    auto mapped_join_inst = dyn_cast<JoinInst>(mapped_memoir_inst);
    MEMOIR_NULL_CHECK(
        mapped_join_inst,
        "JoinInst being visited is no longer a JoinInst after versioning");

    join_inst = mapped_join_inst;
  }

  // Build a new expression to slice the remaining joined collections, if
  // the right index is greater than the size of the leftern collections.
  // Build an expression of the form:
  //   if (size(a) >= R) return slice(a, 0, R);
  //   else if (size(b) >= (R - size(a)))
  //     return join(a, slice(b, 0, (R-size(a))));
  //   ...

  // Get the LLVM JoinInst
  auto &target_inst = join_inst->getCallInst();

  // Create a MemOIRBuilder.
  MemOIRBuilder builder(&target_inst);

  // Invariant state.
  auto &llvm_context = this->M.getContext();
  auto *constant_zero = builder.getInt64(0);

  // Recurrent state.
  llvm::Instruction *split_point = &target_inst;
  llvm::Value *rhs = materialized_right_index;
  llvm::Instruction *last_llvm_size_inst;
  llvm::PHINode *last_phi = nullptr;

  // Output state.
  llvm::PHINode *first_phi = nullptr;

  // Iterate over each of the incoming collections (aside from the last one),
  // creating the nested control flow.
  for (auto join_idx = 0; join_idx < join_inst->getNumberOfJoins() - 1;
       join_idx++) {
    // Move the builder.
    builder.SetInsertPoint(split_point);

    // Update the RHS if we are not the leftmost collection.
    if (join_idx > 0) {
      rhs = builder.CreateSub(rhs, last_llvm_size_inst);
    }

    // Get the current collection operand.
    auto *joined_operand = &(join_inst->getJoinedOperand(join_idx));

    // Sink the incoming collection if possible.
    if (auto *joined_inst = dyn_cast<llvm::Instruction>(joined_operand)) {
      println("Attempting to sink: ", *joined_inst);
      if (joined_operand->hasOneUse()) {
        if (!joined_inst->isEHPad() && !isa<PHINode>(joined_operand)) {
          // && !joined_inst->mayThrow()) {
          if (auto joined_call = dyn_cast<llvm::CallBase>(joined_inst)) {
            println("  Sinking to ", *split_point);
            joined_call->moveBefore(split_point);
          } else {
            println("  Not a call, add handling for this instruction type");
          }
        } else {
          println("  Failed attribute check");
        }
      } else {
        println("  Joined operand has more than one use");
      }
    }

    // Get the size of the current collection.
    auto *size_inst = builder.CreateSizeInst(joined_operand);
    auto *llvm_size_inst = &(size_inst->getCallInst());

    // Create the comparison for this level.
    auto *cmp_inst = builder.CreateICmpUGE(llvm_size_inst, rhs);

    // Split the basic block and create the if-then-else.
    llvm::Instruction *if_term;
    llvm::Instruction *else_term;
    llvm::SplitBlockAndInsertIfThenElse(cmp_inst,
                                        split_point,
                                        &if_term,
                                        &else_term);
    MEMOIR_NULL_CHECK(if_term, "Couldn't create the if clause");
    MEMOIR_NULL_CHECK(else_term, "Couldn't create the else clause");
    auto &if_bb =
        sanitize(if_term->getParent(), "Couldn't get the if basic block");
    auto &else_bb =
        sanitize(else_term->getParent(), "Couldn't get the else basic block");

    // Create a PHI in the continue block.
    builder.SetInsertPoint(split_point);
    auto *collection_type = join_inst->getCallInst().getType();
    auto &continue_phi =
        sanitize(builder.CreatePHI(collection_type, 2),
                 "Could not create the PHINode for the outer if-else");

    // If this is the first if-else, save the continue PHI.
    if (!first_phi) {
      first_phi = &continue_phi;
    }

    // In the if block, create the sliced collection.
    // If we are the leftmost collection in the join, only do a slice.
    // Otherwise, create the slice of the current collection and join it to
    // the ones ahead.
    builder.SetInsertPoint(if_term);
    auto &slice_inst =
        sanitize(builder.CreateSliceInst(joined_operand, constant_zero, rhs),
                 "Could not create the slice instruction");
    auto *collection_inst = &(slice_inst.getCallInst());
    if (join_idx > 0) {
      // Get the leftern collections.
      vector<llvm::Value *> collections_to_join = {};
      for (auto i = 0; i < join_idx; i++) {
        collections_to_join.push_back(&(join_inst->getJoinedOperand(i)));
      }
      collections_to_join.push_back(collection_inst);

      // Build the join inst.
      auto &join_inst = sanitize(builder.CreateJoinInst(collections_to_join),
                                 "Could not create the join instruction");
      collection_inst = &(join_inst.getCallInst());
    }

    // If this is the second to last collection, create the slice and join for
    // the last collection in the else statement. Otherwise, create the PHI for
    // the else and go again.
    llvm::Instruction *else_value;
    if (join_idx == join_inst->getNumberOfJoins() - 2) {
      // Move the builder to the else basic block.
      builder.SetInsertPoint(else_term);

      // Get the final collection.
      auto &final_joined_operand =
          join_inst->getJoinedOperand(join_inst->getNumberOfJoins() - 1);

      // Sink the final collection if possible.
      if (auto *joined_inst =
              dyn_cast<llvm::Instruction>(&final_joined_operand)) {
        println("Attempting to sink: ", *joined_inst);
        if (joined_inst->hasOneUse()) {
          // there is a fake use sitting around still.
          if (!joined_inst->isEHPad() && !isa<PHINode>(joined_operand)) {
            // && !joined_inst->mayThrow()) {
            if (auto joined_call = dyn_cast<llvm::CallBase>(joined_inst)) {
              println("  Sinking to ", *else_term);
              joined_call->moveBefore(else_term);
            } else {
              println("  Not a call, add handling for this instruction type");
            }
          } else {
            println("  Failed attribute check");
          }
        } else {
          println("  Joined operand has more than one use");
        }
      }

      // Create the size inst.
      auto &final_size = sanitize(builder.CreateSizeInst(&final_joined_operand),
                                  "Couldn't create the final size");

      // Create the subtract.
      auto &final_sub_inst =
          sanitize(builder.CreateSub(rhs, &(final_size.getCallInst())),
                   "Couldn't create the final sub");

      // Create the slice.
      auto &final_slice_inst =
          sanitize(builder.CreateSliceInst(&final_joined_operand,
                                           constant_zero,
                                           &final_sub_inst),
                   "Could not create the slice instruction");

      // Create the join.
      vector<llvm::Value *> collections_to_join = {};
      for (auto i = 0; i < join_idx + 1; i++) {
        collections_to_join.push_back(&(join_inst->getJoinedOperand(i)));
      }
      collections_to_join.push_back(&(final_slice_inst.getCallInst()));

      // Build the join inst.
      auto &final_join_inst =
          sanitize(builder.CreateJoinInst(collections_to_join),
                   "Could not create the join instruction");
      auto *final_llvm_join_inst = &(final_join_inst.getCallInst());

      // Update the continue PHI.
      continue_phi.addIncoming(final_llvm_join_inst,
                               final_llvm_join_inst->getParent());
    }

    // Update the PHI in the continue basic block.
    continue_phi.addIncoming(collection_inst, &if_bb);

    // Update the last PHI.
    if (last_phi) {
      last_phi->addIncoming(&continue_phi, continue_phi.getParent());
    }

    // Save the state for the next iteration.
    split_point = else_term;
    last_llvm_size_inst = llvm_size_inst;
    last_phi = &continue_phi;
  }

  // Mark the original join for deletion.
  this->markForDeletion(target_inst);

  // Return.
  return first_phi;
}

llvm::Value *SlicePropagation::visitSliceInst(SliceInst &I) {
  // Get the information for how this instruction should be sliced.
  auto &llvm_inst = I.getCallInst();
  auto left_index = this->candidate->left_index;
  auto right_index = this->candidate->right_index;

  // Check that the left index is a constant zero.
  auto constant_int = as<llvm::ConstantInt>(left_index);
  if (!constant_int) {
    println("Slicing a join with a non-constant left index "
            " is unsupported at the moment");
    return nullptr;
  }

  if (constant_int->getZExtValue() != 0) {
    println("Slicing a join with the left index "
            "being non-zero is unsupported at the moment.");
    return nullptr;
  }

  // See if this slice instruction has the same range as the slice we are
  // looking at. If it does, replace the JoinInst with its first operand.
  auto &candidate_left_index = I.getBeginIndex();
  auto &candidate_right_index = I.getEndIndex();

  // Check that the left and right indices of the slice range are the same
  // as the slice being propagated;
  if ((*left_index == candidate_left_index)
      && (*right_index == candidate_right_index)) {
    return &llvm_inst;
  }

  return nullptr;
}

// Internal helpers.
bool SlicePropagation::checkVisited(llvm::Value &V) {
  return (this->visited.find(&V) != this->visited.end());
}

void SlicePropagation::recordVisit(llvm::Value &V) {
  this->visited.insert(&V);
}

void SlicePropagation::markForDeletion(llvm::Value &V) {
  this->values_to_delete.insert(&V);
}

} // namespace llvm::memoir
