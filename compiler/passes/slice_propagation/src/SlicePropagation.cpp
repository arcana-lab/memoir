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
  println("SlicePropagation: performing analysis");

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
        auto user = use.getUser();
        if (user != &llvm_slice_inst) {
          slice_is_only_user = false;
          break;
        }
      }

      if (!slice_is_only_user) {
        println("Slice is not the only user.");
        continue;
      }

      // Initialize the state.
      this->slice_under_test = slice_inst;

      // Look at the sliced collection to see if it can be sliced.
      auto &sliced_collection = slice_inst->getCollection();
      this->visitCollection(sliced_collection);
    }
  }

  println("SlicePropagation: done analyzing");

  return true;
}

// Top-level transformation.
bool SlicePropagation::transform() {
  println("SlicePropagation: begin transform");

  // For each slice candidate, attempt to propagate the slice to it.
  for (auto const &[collection, slice_range] : this->collections_to_slice) {
    println("Slicing");
    println("  collection: ", *collection);
    println("        left: ", *(slice_range.first));
    println("       right: ", *(slice_range.second));
    if (auto argument = dyn_cast<llvm::Argument>(collection)) {
      // We have an LLVM argument.
      println("Visiting an argument");
      this->visitArgument(*argument);
    } else if (auto inst = dyn_cast<llvm::Instruction>(collection)) {
      // We have an LLVM instruction.
      println("Visiting an instruction");
      println(*inst);
      this->visit(*inst);
    }
  }

  println("SlicePropagation: end transform");

  return false;
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

  // Mark the collection allocation for slicing.
  auto &left_slice = this->slice_under_test->getBeginIndex();
  auto &right_slice = this->slice_under_test->getEndIndex();
  this->collections_to_slice[&llvm_alloc_inst] =
      make_pair(&left_slice, &right_slice);

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
  println("  ", C.getPHI());

  // Check if we have already visited this instruction.
  CHECK_VISITED(C.getPHI());

  // Iterate through all incoming edges.
  bool marked = false;
  for (auto idx = 0; idx < C.getNumIncoming(); idx++) {
    auto &incoming = C.getIncomingCollection(idx);
    println("  incoming ", std::to_string(idx), ": ", incoming);
    marked |= this->visitCollection(incoming);
  }

  return marked;
}

bool SlicePropagation::visitRetPHICollection(RetPHICollection &C) {
  println("Visiting a return PHI collection.");

  // Check if we have already visited this instruction.
  CHECK_VISITED(C.getCall());

  // Iterate through all possible return values.
  // NOTE: This should be clean with noelle's normalization providing a single
  // return point for all functions.
  bool marked = false;
  for (auto ret_idx = 0; ret_idx < C.getNumIncoming(); ret_idx++) {
    auto &incoming = C.getIncomingCollection(ret_idx);
    println("  incoming ", std::to_string(ret_idx), ": ", incoming);
    marked |= this->visitCollection(incoming);
  }

  return marked;
}

bool SlicePropagation::visitArgPHICollection(ArgPHICollection &C) {
  println("Visiting an argument PHI collection.");

  // Check if we have already visited this instruction.
  CHECK_VISITED(C.getArgument());

  // Iterate through all incoming arguments.
  bool marked = false;
  for (auto idx = 0; idx < C.getNumIncoming(); idx++) {
    auto &incoming = C.getIncomingCollection(idx);
    println("  incoming ", std::to_string(idx), ": ", incoming);
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

  // Check if we have already visited this instruction.
  CHECK_VISITED(llvm_join_inst);

  // Get the slice information.
  auto &left_index = this->slice_under_test->getBeginIndex();
  auto &right_index = this->slice_under_test->getEndIndex();

  // If the first index is a constant zero, recurse on the first
  // operand of the Join.
  // TODO: recurse on the last operand if the index is a constant -1.
  if (auto left_index_as_constant_int =
          dyn_cast<llvm::ConstantInt>(&left_index)) {
    if (left_index_as_constant_int->getZExtValue() == 0) {
      if (this->visitCollection(C.getJoinedCollection(0))) {
        return true;
      }
    }
  }

  // Mark the joined collection as being prepared for slicing.
  this->collections_to_slice[&llvm_join_inst] =
      make_pair(&left_index, &right_index);

  return false;
}

bool SlicePropagation::visitSliceCollection(SliceCollection &C) {
  println("Visiting a slice collection.");

  // Get the MemOIR Inst and the LLVM CallInst.
  auto &slice_inst = C.getSlice();
  auto &llvm_slice_inst = slice_inst.getCallInst();

  // Check if we have already visited this instruction.
  CHECK_VISITED(llvm_slice_inst);

  // Get the slice range.
  auto &left_index = this->slice_under_test->getBeginIndex();
  auto &right_index = this->slice_under_test->getEndIndex();

  // Mark the collection being sliced if the left index of this
  // SliceCollection is a constant 0.
  // TODO: extend the above case to handle masking the slice further and
  // applying it to the sliced collection.
  if (auto left_index_as_constant_int =
          dyn_cast<llvm::ConstantInt>(&left_index)) {
    if (left_index_as_constant_int->getZExtValue() == 0) {
      if (this->visitCollection(C.getSlicedCollection())) {
        return true;
      }
    }
  }

  // Mark the joined collection as being prepared for slicing.
  this->collections_to_slice[&llvm_slice_inst] =
      make_pair(&left_index, &right_index);

  return true;
}

// InstVisitor methods for transformation.
llvm::Value *SlicePropagation::visitArgument(llvm::Argument &A) {
  return nullptr;
}

llvm::Value *SlicePropagation::visitInstruction(llvm::Instruction &I) {
  return nullptr;
}

llvm::Value *SlicePropagation::visitSequenceAllocInst(SequenceAllocInst &I) {
  // Get the information for how this instruction should be sliced.
  auto &llvm_inst = I.getCallInst();
  auto slice_pair = this->collections_to_slice[&llvm_inst];
  auto left_index = slice_pair.first;
  auto right_index = slice_pair.second;

  // Perform some sanity checks.
  MEMOIR_NULL_CHECK(right_index,
                    "Right index of slice being propagated is NULL!");
  MEMOIR_NULL_CHECK(left_index,
                    "Left index of slice being propagated is NULL!");

  // Check that the left index is a constant zero.
  auto constant_int = dyn_cast<ConstantInt>(left_index);
  if (!constant_int) {
    println("Slicing an allocation with a non-constant left index "
            " is unsupported at the moment");
    return nullptr;
  }

  if (constant_int->getZExtValue() != 0) {
    println("Slicing an allocation with the left index "
            "being non-zero is unsupported at the moment.");
    return nullptr;
  }

  // Get the size operand of the allocation instruction.
  auto &size_operand_as_use = I.getSizeOperandAsUse();

  // Check that the allocation and slice operand are both available in the same
  // function.
  // TODO: add function versioning to elide this issue.
  auto alloc_bb = llvm_inst.getParent();
  MEMOIR_NULL_CHECK(alloc_bb,
                    "SequenceAllocInst is not attached to a basic block!");
  auto alloc_func = alloc_bb->getParent();
  MEMOIR_NULL_CHECK(alloc_func,
                    "SequenceAllocInst is not attached to a function!");
  if (auto right_index_as_arg = dyn_cast<llvm::Argument>(right_index)) {
    auto right_index_func = right_index_as_arg->getParent();
    if (right_index_func != alloc_func) {
      println("Right index is an argument of a function that "
              "doesn't contain the allocation!");
      return nullptr;
    }
  } else if (auto right_index_as_constant =
                 dyn_cast<llvm::ConstantInt>(right_index)) {
    println("Right index is a constant, proceed.");
  } else if (auto right_index_as_inst =
                 dyn_cast<llvm::Instruction>(right_index)) {
    auto right_index_bb = right_index_as_inst->getParent();
    MEMOIR_NULL_CHECK(right_index_bb,
                      "Right index is not attached to a basic block!");
    auto right_index_func = right_index_bb->getParent();
    MEMOIR_NULL_CHECK(right_index_func,
                      "Right index is not attached to a function!");
    // FIXME: add argument propagation here so that we can access the value iff
    // it dominates call we care about.
    if (right_index_func != alloc_func) {
      println("Right index is an instruction in a different function "
              "from the allocation!");
      return nullptr;
    }

    // Check that the right slice index instruction dominates the allocation.
    // TODO: we may not need to recalculate the dominator tree each time here.
    auto &DTA = this->P.getAnalysis<DominatorTreeWrapperPass>(*alloc_func);
    auto const &DT = DTA.getDomTree();

    if (!DT.dominates(right_index_as_inst, size_operand_as_use)) {
      println("Right index of slice does not dominate the allocation.");
      return nullptr;
    }
  }

  println("Transforming: ", I);

  // Replace the size operand of the allocation instruction with the right
  // index value.
  size_operand_as_use.set(right_index);

  println("Transformed: ", I);

  return &llvm_inst;
}

llvm::Value *SlicePropagation::visitJoinInst(JoinInst &I) {
  // Get the information for how this instruction should be sliced.
  auto &llvm_inst = I.getCallInst();
  auto slice_pair = this->collections_to_slice[&llvm_inst];
  auto left_index = slice_pair.first;
  auto right_index = slice_pair.second;

  // Perform some sanity checks.
  MEMOIR_NULL_CHECK(left_index,
                    "Left index of slice being propagated is NULL!");
  MEMOIR_NULL_CHECK(right_index,
                    "Right index of slice being propagated is NULL!");

  println("Slicing ", I);
  println("   left = ", *left_index);
  println("  right = ", *right_index);

  // Check that the left index is a constant zero.
  auto constant_int = dyn_cast<ConstantInt>(left_index);
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

  // If the first operand of the join is a slice inst, see if it has the same
  // range as the slice we are looking at.
  // If it does, replace the JoinInst with its first operand.
  auto &first_operand_as_use = I.getJoinedOperandAsUse(0);
  auto first_operand_as_value = first_operand_as_use.get();
  MEMOIR_NULL_CHECK(first_operand_as_value,
                    "First operand of slice instruction is NULL!");

  if (auto first_operand_as_inst =
          dyn_cast<llvm::Instruction>(first_operand_as_use)) {
    if (auto first_operand_as_memoir =
            MemOIRInst::get(*first_operand_as_inst)) {
      if (auto first_operand_as_slice =
              dyn_cast<SliceInst>(first_operand_as_memoir)) {
        // Get the slice range.
        auto &operand_left_index = first_operand_as_slice->getBeginIndex();
        auto &operand_right_index = first_operand_as_slice->getEndIndex();

        // Check that the left and right indices of the slice range are the same
        // as the slice being propagated;
        if ((&operand_left_index == left_index)
            && (&operand_right_index == right_index)) {
          // Replace the JoinInst with its first operand.
          llvm_inst.replaceAllUsesWith(first_operand_as_value);
          return first_operand_as_value;
        }
      }
    }
  }

  return nullptr;
}

llvm::Value *SlicePropagation::visitSliceInst(SliceInst &I) {
  // Get the information for how this instruction should be sliced.
  auto &llvm_inst = I.getCallInst();
  auto slice_pair = this->collections_to_slice[&llvm_inst];
  auto left_index = slice_pair.first;
  auto right_index = slice_pair.second;

  // Perform some sanity checks.
  MEMOIR_NULL_CHECK(left_index,
                    "Left index of slice being propagated is NULL!");
  MEMOIR_NULL_CHECK(right_index,
                    "Right index of slice being propagated is NULL!");

  // Check that the left index is a constant zero.
  auto constant_int = dyn_cast<ConstantInt>(left_index);
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
  if ((&candidate_left_index == left_index)
      && (&candidate_right_index == right_index)) {
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

} // namespace llvm::memoir
