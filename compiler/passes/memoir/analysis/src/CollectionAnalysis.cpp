#include "memoir/analysis/CollectionAnalysis.hpp"

#include "memoir/support/Print.hpp"

namespace llvm::memoir {

/*
 * Initialization.
 */
CollectionAnalysis::CollectionAnalysis(llvm::noelle::Noelle &noelle,
                                       bool enable_def_phi,
                                       bool enable_use_phi)
  : noelle(noelle),
    enable_def_phi(enable_def_phi),
    enable_use_phi(enable_use_phi) {
  // Do nothing.
}

CollectionAnalysis::CollectionAnalysis(llvm::noelle::Noelle &noelle)
  : CollectionAnalysis(noelle, false, false) {
  // Do nothing
}

/*
 * Queries
 */
Collection *CollectionAnalysis::analyze(llvm::Use &U) {
  return CollectionAnalysis::get().getCollection(U);
}

Collection *CollectionAnalysis::analyze(llvm::Value &V) {
  return CollectionAnalysis::get().getCollection(V);
}

/*
 * Analysis
 */
Collection *CollectionAnalysis::getCollection(llvm::Use &U) {
  // Get the value being used.
  auto used = U.get();
  MEMOIR_NULL_CHECK(used, "Used value for use is NULL!");

  // If we have disabled USE and DEF PHIs, analyze the value.
  if (!this->enable_use_phi && !this->enable_def_phi) {
    return this->getCollection(*used);
  }

  MEMOIR_UNREACHABLE(
      "USE PHI and DEF PHI are currently unsupported, please disable them.");

  // Inspect all uses of the same value to construct a path-sensitive analysis.
  auto user = U.getUser();
  MEMOIR_NULL_CHECK(user, "User for use is NULL!");

  auto user_as_inst = dyn_cast<llvm::Instruction>(user);

  // We don't know what this is, return NULL.
  if (!user_as_inst) {
    return nullptr;
  }

  // Get the current function parent.
  auto bb = user_as_inst->getParent();
  MEMOIR_NULL_CHECK(bb, "User is not attached to a basic block");

  auto function = bb->getParent();
  MEMOIR_NULL_CHECK(function, "User is not attached to a function");

  // Get the NOELLE dominator summary.
  auto &DS = this->getDominators(*function);

  // Get the NOELLE dominator tree.
  auto &DT = DS.DT;

  // Gather all of the fellow users, filter out users in the same basic block as
  // this which are dominated by the user being analyzed.
  std::set<llvm::Instruction *> fellow_users = {};
  for (auto fellow_user : used->users()) {
    auto fellow_user_as_inst = dyn_cast<llvm::Instruction>(fellow_user);
    if (fellow_user_as_inst == user_as_inst) {
      // Properly dominates
      continue;
    } else if (fellow_user_as_inst->getParent() == bb) {
      if (DT.dominates(user_as_inst, fellow_user_as_inst)) {
        // Filter fellow users within the block that we dominate.
        continue;
      }
    } else {
      fellow_users.insert(fellow_user_as_inst);
    }
  }

  // Find all fellow users that dominate this user's basic block.
  // auto dominators = DT.getDominatorsOf(fellow_users, bb);

  // If this is a MemOIRInst, handle it.
  if (auto memoir_inst = MemOIRInst::get(*user_as_inst)) {
    if (auto read_inst = dyn_cast<ReadInst>(memoir_inst)) {
      // TODO
      return nullptr;
    } else if (auto write_inst = dyn_cast<WriteInst>(memoir_inst)) {
      // TODO
      return nullptr;
    } else {
      // TODO
    }
  }

  // If this is simply an LLVM Instruction, handle it.

  return nullptr;
}

Collection *CollectionAnalysis::getCollection(llvm::Value &V) {
  /*
   * Find the associated memoir collection, if it exists.
   */

  /*
   * If we have an instruction, visit it.
   */
  if (auto inst = dyn_cast<llvm::Instruction>(&V)) {
    return this->visit(*inst);
  }

  /*
   * If we have an argument, visit it.
   */
  if (auto arg = dyn_cast<llvm::Argument>(&V)) {
    return this->visitArgument(*arg);
  }

  /*
   * Otherwise, we don't know how to handle this Value, return NULL.
   */
  return nullptr;
}

/*
 * Helper macros
 */
#define CHECK_MEMOIZED(V)                                                      \
  /* See if an existing collection exists, if it does, early return. */        \
  if (auto found = this->findExisting(V)) {                                    \
    return found;                                                              \
  }

#define MEMOIZE_AND_RETURN(V, C)                                               \
  /* Memoize the collection */                                                 \
  this->memoize(V, C);                                                         \
  /* Return */                                                                 \
  return C

/*
 * Visitor methods
 */
Collection *CollectionAnalysis::visitInstruction(llvm::Instruction &I) {
  CHECK_MEMOIZED(I);

  // Do nothing.

  MEMOIZE_AND_RETURN(I, nullptr);
}

Collection *CollectionAnalysis::visitStructAllocInst(StructAllocInst &I) {
  CHECK_MEMOIZED(I);

  // TODO

  MEMOIZE_AND_RETURN(I, nullptr);
}

Collection *CollectionAnalysis::visitTensorAllocInst(TensorAllocInst &I) {
  CHECK_MEMOIZED(I);

  auto tensor_alloc_collection = new BaseCollection(I);

  MEMOIZE_AND_RETURN(I, tensor_alloc_collection);
}

Collection *CollectionAnalysis::visitSequenceAllocInst(SequenceAllocInst &I) {
  CHECK_MEMOIZED(I);

  auto sequence_alloc_collection = new BaseCollection(I);

  MEMOIZE_AND_RETURN(I, sequence_alloc_collection);
}

Collection *CollectionAnalysis::visitAssocArrayAllocInst(
    AssocArrayAllocInst &I) {
  CHECK_MEMOIZED(I);

  auto assoc_array_alloc_collection = new BaseCollection(I);

  MEMOIZE_AND_RETURN(I, assoc_array_alloc_collection);
}

Collection *CollectionAnalysis::visitGetInst(GetInst &I) {
  CHECK_MEMOIZED(I);

  auto nested_collection = new NestedCollection(I);

  MEMOIZE_AND_RETURN(I, nested_collection);
}

Collection *CollectionAnalysis::visitReadInst(ReadInst &I) {
  CHECK_MEMOIZED(I);

  /*
   * Check the type of the ReadInst, if it's not a
   * collection reference type, return NULL.
   */
  auto &accessed_type = I.getCollectionType().getElementType();
  if (!Type::is_reference_type(accessed_type)) {
    return nullptr;
  }

  auto &reference_type = static_cast<ReferenceType &>(accessed_type);
  auto &referenced_type = reference_type.getReferencedType();

  if (!Type::is_collection_type(referenced_type)) {
    return nullptr;
  }

  auto referenced_collection = new ReferencedCollection(I);

  MEMOIZE_AND_RETURN(I, referenced_collection);
}

Collection *CollectionAnalysis::visitJoinInst(JoinInst &I) {
  CHECK_MEMOIZED(I);

  auto join_phi_collection = new JoinPHICollection(I);

  MEMOIZE_AND_RETURN(I, join_phi_collection);
}

Collection *CollectionAnalysis::visitSliceInst(SliceInst &I) {
  CHECK_MEMOIZED(I);

  auto slice_collection = new SliceCollection(I);

  MEMOIZE_AND_RETURN(I, slice_collection);
}

Collection *CollectionAnalysis::visitPHINode(llvm::PHINode &I) {
  CHECK_MEMOIZED(I);

  // Get the current PHI index, if it exists.
  unsigned current_index = 0;
  auto found_current_index = this->current_phi_index.find(&I);
  if (found_current_index != this->current_phi_index.end()) {
    current_index = found_current_index->second;
  }

  // Iterate through the incoming edges to find the collection.
  bool has_collection = false;
  map<llvm::BasicBlock *, Collection *> incoming = {};
  for (auto idx = current_index; idx < I.getNumIncomingValues(); idx++) {
    auto incoming_bb = I.getIncomingBlock(idx);
    MEMOIR_NULL_CHECK(incoming_bb, "Incoming basic block for PHINode is NULL!");

    auto &incoming_use = I.getOperandUse(idx);

    // Before we recurse, save the current PHI index so we don't enter an
    // infinite loop.
    this->current_phi_index[&I] = idx + 1;

    // Recurse.
    auto incoming_collection = this->getCollection(*incoming_use);
    if (incoming_collection == nullptr) {
      if (!has_collection) {
        MEMOIZE_AND_RETURN(I, nullptr);
      } else {
        MEMOIR_UNREACHABLE(
            "Could not determine the incoming collection for a PHINode that "
            "has incoming collections on other edges.");
      }
    } else {
      has_collection = true;
      incoming[incoming_bb] = incoming_collection;
    }
  }

  // TODO: May need to add some patching in here, so that if any index was
  // skipped because it's an element of an SCC, it will be logged correctly.

  // Clear the memoized indices.
  found_current_index = this->current_phi_index.find(&I);
  if (found_current_index != this->current_phi_index.end()) {
    this->current_phi_index.erase(found_current_index);
  }

  // Create a new control PHI.
  auto control_phi_collection = new ControlPHICollection(I, incoming);

  MEMOIZE_AND_RETURN(I, control_phi_collection);
}

Collection *CollectionAnalysis::visitLLVMCallInst(llvm::CallInst &I) {
  CHECK_MEMOIZED(I);

  map<llvm::ReturnInst *, Collection *> incoming = {};
  auto callee = I.getCalledFunction();
  if (callee) {
    // Handle direct call.
    for (auto &BB : *callee) {
      auto terminator = BB.getTerminator();
      if (auto return_inst = dyn_cast<llvm::ReturnInst>(terminator)) {
        auto incoming_collection = this->visitReturnInst(*return_inst);
        incoming[return_inst] = incoming_collection;
      }
    }
  } else {
    // Handle indirect call.
    auto function_type = I.getFunctionType();
    MEMOIR_NULL_CHECK(function_type, "Found a call with NULL function type");

    auto parent_bb = I.getParent();
    MEMOIR_NULL_CHECK(parent_bb,
                      "Could not determine the parent basic block of the call");
    auto parent_function = parent_bb->getParent();
    MEMOIR_NULL_CHECK(parent_function,
                      "Could not determine the parent function of the call");
    auto parent_module = parent_function->getParent();
    MEMOIR_NULL_CHECK(parent_module,
                      "Could not determine the parent module of the call");

    for (auto &F : *parent_module) {
      if (F.getFunctionType() != function_type) {
        continue;
      }

      for (auto &BB : F) {
        auto terminator = BB.getTerminator();
        if (auto return_inst = dyn_cast<llvm::ReturnInst>(terminator)) {
          auto incoming_collection = this->visitReturnInst(*return_inst);
          incoming[return_inst] = incoming_collection;
        }
      }
    }
  }

  bool found_collection = false;
  for (auto const &[return_inst, incoming_collection] : incoming) {
    if (incoming_collection) {
      found_collection = true;
      break;
    }
  }

  if (!found_collection) {
    MEMOIZE_AND_RETURN(I, nullptr);
  }

  auto ret_phi_collection = new RetPHICollection(I, incoming);

  MEMOIZE_AND_RETURN(I, ret_phi_collection);
}

Collection *CollectionAnalysis::visitReturnInst(llvm::ReturnInst &I) {
  CHECK_MEMOIZED(I);

  auto &returned_use = I.getOperandUse(0);

  auto returned_collection = this->getCollection(returned_use);

  MEMOIZE_AND_RETURN(I, returned_collection);
}

Collection *CollectionAnalysis::visitArgument(llvm::Argument &A) {
  CHECK_MEMOIZED(A);

  map<llvm::CallBase *, Collection *> incoming;

  auto parent_function = A.getParent();
  MEMOIR_NULL_CHECK(parent_function,
                    "Could not determine the parent function of the Argument");

  auto parent_module = parent_function->getParent();
  MEMOIR_NULL_CHECK(parent_module,
                    "Could not determine the parent module of the Argument");

  auto parent_function_type = parent_function->getFunctionType();
  MEMOIR_NULL_CHECK(parent_function_type,
                    "Function type of Argument's parent function is NULL!");

  /*
   * Iterate over all Call's in the program to find possible calls to this
   * function.
   */
  for (auto &F : *parent_module) {
    for (auto &BB : F) {
      for (auto &I : BB) {
        auto call_base = dyn_cast<llvm::CallBase>(&I);

        // Ignore non-call instructions.
        if (!call_base) {
          continue;
        }

        // Check the called function.
        auto callee = call_base->getCalledFunction();
        if (callee == parent_function) {
          // Handle direct calls.
          auto &call_argument_as_use =
              call_base->getArgOperandUse(A.getArgNo());
          auto argument_collection = this->getCollection(call_argument_as_use);
          MEMOIR_NULL_CHECK(argument_collection,
                            "Incoming collection to Argument is NULL!");

          incoming[call_base] = argument_collection;
        } else if (!callee) {
          // Handle indirect calls.
          auto function_type = callee->getFunctionType();
          if (function_type == parent_function_type) {
            auto &call_argument_as_use =
                call_base->getArgOperandUse(A.getArgNo());
            auto argument_collection =
                this->getCollection(call_argument_as_use);
            MEMOIR_NULL_CHECK(
                argument_collection,
                "Incoming collection to Argument from indirect call is NULL!");

            incoming[call_base] = argument_collection;
          }
        }
      }
    }
  }

  auto arg_phi_collection = new ArgPHICollection(A, incoming);

  MEMOIZE_AND_RETURN(A, arg_phi_collection);
}

/*
 * Internal helper functions
 */
Collection *CollectionAnalysis::findExisting(llvm::Value &V) {
  auto found_collection = this->value_to_collection.find(&V);
  if (found_collection != this->value_to_collection.end()) {
    return found_collection->second;
  }

  return nullptr;
}

Collection *CollectionAnalysis::findExisting(MemOIRInst &I) {
  return this->findExisting(I.getCallInst());
}

Collection *CollectionAnalysis::findExisting(llvm::Use &U) {
  auto found_collection = this->use_to_collection.find(&U);
  if (found_collection != this->use_to_collection.end()) {
    return found_collection->second;
  }

  return nullptr;
}

void CollectionAnalysis::memoize(llvm::Value &V, Collection *C) {
  this->value_to_collection[&V] = C;
}

void CollectionAnalysis::memoize(MemOIRInst &I, Collection *C) {
  this->memoize(I.getCallInst(), C);
}

void CollectionAnalysis::memoize(llvm::Use &U, Collection *C) {
  this->use_to_collection[&U] = C;
}

llvm::noelle::DominatorSummary &CollectionAnalysis::getDominators(
    llvm::Function &F) {
  // See if we already have a dominator summary for this
  // Function. If we do, return it.
  auto found_dominator = this->function_to_dominator_summary.find(&F);
  if (found_dominator != this->function_to_dominator_summary.end()) {
    return *(found_dominator->second);
  }

  // Otherwise, let's ask NOELLE for it.
  auto dominator_summary = noelle.getDominators(&F);
  MEMOIR_NULL_CHECK(dominator_summary,
                    "Could not get DominatorSummary from NOELLE");

  // Memoize and return.
  this->function_to_dominator_summary[&F] = dominator_summary;

  return *dominator_summary;
}

/*
 * Management
 */
CollectionAnalysis *CollectionAnalysis::CA = nullptr;

CollectionAnalysis &CollectionAnalysis::get(Noelle &noelle) {
  if (CollectionAnalysis::CA == nullptr) {
    CollectionAnalysis::CA = new CollectionAnalysis(noelle, false, false);
  }
  return *(CollectionAnalysis::CA);
}

CollectionAnalysis &CollectionAnalysis::get() {
  MEMOIR_NULL_CHECK(
      CollectionAnalysis::CA,
      "You must construct the CollectionAnalysis in your pass first,"
      " giving it a Noelle instance");
  return *(CollectionAnalysis::CA);
}

void CollectionAnalysis::invalidate() {
  CollectionAnalysis::get()._invalidate();
}

void CollectionAnalysis::_invalidate() {
  return;
}

} // namespace llvm::memoir
