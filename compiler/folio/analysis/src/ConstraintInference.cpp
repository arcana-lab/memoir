#include "memoir/support/Casting.hpp"

#include "folio/analysis/ConstraintInference.hpp"

using namespace llvm::memoir;

namespace folio {

void ConstraintInferenceDriver::init() {
  for (auto &F : this->M) {
    if (F.empty()) {
      continue;
    }

    for (auto &BB : F) {
      for (auto &I : BB) {

        // TODO: check if the instruction should be considered hot, if so apply
        // the FastOperationConstraint as well.
        // Set the is_hot variable to true if that is the case.
        auto is_hot = false;

        if (auto *read = into<ReadInst>(&I)) {
          auto &collection = read->getObjectOperand();
          auto *type = type_of(collection);

          if (isa_and_nonnull<SequenceType>(type)
              or isa_and_nonnull<AssocArrayType>(type)) {

            this->constraints.add(collection, OperationConstraint<ReadInst>());
            if (is_hot) {
              this->constraints.add(collection,
                                    FastOperationConstraint<ReadInst>());
            }
          }

        } else if (auto *write = into<WriteInst>(&I)) {
          auto *type = type_of(*write);

          if (isa_and_nonnull<SequenceType>(type)
              or isa_and_nonnull<AssocArrayType>(type)) {

            this->constraints.add(I, OperationConstraint<WriteInst>());
            if (is_hot) {
              this->constraints.add(I, FastOperationConstraint<WriteInst>());
            }
          }
        } else if (into<InsertInst>(&I)) {
          this->constraints.add(I, OperationConstraint<InsertInst>());
          if (is_hot) {
            this->constraints.add(I, FastOperationConstraint<InsertInst>());
          }
        } else if (into<RemoveInst>(&I)) {
          this->constraints.add(I, OperationConstraint<RemoveInst>());
          if (is_hot) {
            this->constraints.add(I, FastOperationConstraint<RemoveInst>());
          }
        } else if (into<SwapInst>(&I)) {
          this->constraints.add(I, OperationConstraint<SwapInst>());
          if (is_hot) {
            this->constraints.add(I, FastOperationConstraint<SwapInst>());
          }
        } else if (auto *has = into<AssocHasInst>(&I)) {
          auto &collection = has->getObjectOperand();

          this->constraints.add(collection,
                                OperationConstraint<AssocHasInst>());
          if (is_hot) {
            this->constraints.add(collection,
                                  FastOperationConstraint<AssocHasInst>());
          }
        } else if (auto *fold = into<FoldInst>(&I)) {
          auto &collection = fold->getCollection();

          this->constraints.add(collection, OperationConstraint<FoldInst>());
          if (is_hot) {
            this->constraints.add(collection,
                                  FastOperationConstraint<FoldInst>());
          }
        } else if (auto *rfold = into<ReverseFoldInst>(&I)) {
          auto &collection = rfold->getCollection();

          this->constraints.add(collection,
                                OperationConstraint<ReverseFoldInst>());
          if (is_hot) {
            this->constraints.add(collection,
                                  FastOperationConstraint<ReverseFoldInst>());
          }
        }

        // TODO: check if we have references live across operations that impact
        // the index space.
      }
    }
  }

  return;
}

bool ConstraintInferenceDriver::propagate(llvm::Value &V, ConstraintSet &C) {
  // Get the constraints for V.
  auto &V_C = this->constraints.value_to_constraints[&V];

  // Save the original size so that we can check for modification later.
  auto original_size = V_C.size();

  // Insert the incoming constraints into V_C
  V_C.insert(C.begin(), C.end());

  // Return true if the size changed.
  return original_size != V_C.size();
}

void ConstraintInferenceDriver::infer() {

  // Unpack the underlying constraint mapping.
  map<llvm::Value *, ConstraintSet> &mapping =
      this->constraints.value_to_constraints;

  // Construct a mapping from functions to their possible callees.
  map<llvm::Function *, ordered_set<llvm::CallBase *>> possible_callers = {};
  map<llvm::CallBase *, ordered_set<llvm::Function *>> possible_callees = {};

  for (auto &F : M) {
    if (F.empty()) {
      continue;
    }

    for (auto &BB : F) {
      for (auto &I : BB) {
        if (auto *call = dyn_cast<llvm::CallBase>(&I)) {
          if (auto *fold = into<FoldInst>(&I)) {

            auto &function = fold->getFunction();
            possible_callers[&function].insert(call);

            continue;
          }

          // If it is a direct call, record it.
          if (auto *called_function = call->getCalledFunction()) {
            possible_callers[called_function].insert(call);
            possible_callees[call].insert(called_function);

            continue;
          }

          // Otherwise, find all possible calls.
          auto *function_type = call->getFunctionType();

          // A call may occur if the function and call share a function type.
          // NOTE: this can be made less naive with NOELLE's complete call
          // graph, or points-to analysis
          for (auto &OtherF : M) {
            if (OtherF.getFunctionType() == function_type) {
              possible_callers[&OtherF].insert(call);
              possible_callees[call].insert(&OtherF);
            }
          }
        }
      }
    }
  }

  // Gather the initial worklist.
  vector<llvm::Value *> worklist;
  worklist.reserve(this->constraints.value_to_constraints.size());

  for (auto [val, c] : this->constraints.value_to_constraints) {
    worklist.push_back(val);
  }

  // Iterate until a fixed-point is reached.
  while (not worklist.empty()) {
    // Pop off of the worklist.
    auto *current = worklist.back();
    worklist.pop_back();

    // Fetch the constraint set for the current value.
    auto &current_constraints = mapping[current];

    // Propagate the constraint set to the used collections.
    if (auto *arg = dyn_cast<llvm::Argument>(current)) {
      // Unpack the argument's information.
      auto arg_no = arg->getArgNo();
      auto *parent_function = arg->getParent();

      // If the current value is an argument, propagate the information to all
      // possible callers.
      for (auto *call : possible_callers[parent_function]) {
        // If the caller is a FoldInst, handle it specially.
        if (auto *fold = into<FoldInst>(call)) {
          // TODO
        }

        // Otherwise, propagate the information to the used value.
        auto &use = call->getOperandUse(arg_no);
        auto &used = MEMOIR_SANITIZE(use.get(), "Used value is NULL");

        // Propagate the constraints to the use.
        auto changed = propagate(used, current_constraints);

        // If the set of constraints changed, add the used variable to the
        // worklist.
        if (changed) {
          worklist.push_back(&used);
        }
      }
    } else if (auto *phi = dyn_cast<llvm::PHINode>(current)) {
      // Propagate the PHI's constraints along all incoming edges.
      for (auto &incoming_use : phi->incoming_values()) {
        auto *incoming = incoming_use.get();

        auto changed = propagate(*incoming, current_constraints);
        if (changed) {
          worklist.push_back(incoming);
        }
      }
    } else if (auto *extract = dyn_cast<llvm::ExtractValueInst>(current)) {
      // NOTE: this is a naive handling of tuples.
      auto changed = propagate(*extract, current_constraints);
      if (changed) {
        worklist.push_back(extract);
      }
    } else if (auto *memoir_inst = into<MemOIRInst>(current)) {
      if (auto *write = dyn_cast<WriteInst>(memoir_inst)) {
        auto &collection = write->getObjectOperand();

        auto changed = propagate(collection, current_constraints);
        if (changed) {
          worklist.push_back(&collection);
        }
      } else if (auto *insert = dyn_cast<InsertInst>(memoir_inst)) {
        auto &collection = insert->getBaseCollection();

        auto changed = propagate(collection, current_constraints);
        if (changed) {
          worklist.push_back(&collection);
        }
      } else if (auto *remove = dyn_cast<RemoveInst>(memoir_inst)) {
        auto &collection = remove->getBaseCollection();

        auto changed = propagate(collection, current_constraints);
        if (changed) {
          worklist.push_back(&collection);
        }
      } else if (auto *swap = dyn_cast<SwapInst>(memoir_inst)) {
        auto &from_collection = swap->getFromCollection();
        auto &to_collection = swap->getToCollection();

        if (propagate(from_collection, current_constraints)) {
          worklist.push_back(&from_collection);
        }

        if (propagate(to_collection, current_constraints)) {
          worklist.push_back(&to_collection);
        }
      } else if (auto *fold = dyn_cast<FoldInst>(memoir_inst)) {
        auto &collection = fold->getCollection();

        if (propagate(collection, current_constraints)) {
          worklist.push_back(&collection);
        }
      } else if (auto *copy = dyn_cast<CopyInst>(memoir_inst)) {
        // NOTE: for simplicity we will force all copies to be the same
        // selection as their input. If this assumption changes, this code needs
        // to change with it!
        auto &copied = copy->getCopiedCollection();

        if (propagate(copied, current_constraints)) {
          worklist.push_back(&copied);
        }
      } else if (auto *get = dyn_cast<GetInst>(memoir_inst)) {
        // NOTE: we do not currently support nested collections, if that
        // assumption changes additional handling needs to be added here.
      } else if (auto *ret_phi = dyn_cast<RetPHIInst>(memoir_inst)) {
        auto &collection = ret_phi->getInput();

        if (propagate(collection, current_constraints)) {
          worklist.push_back(&collection);
        }
      }
    } else if (auto *call = dyn_cast<llvm::CallBase>(current)) {

      // For each possible callee, inspect it.
      for (auto *callee : possible_callees[call]) {

        // Find all return instructions in the function and propagate
        // constraints to the returned value.
        for (auto &BB : *callee) {
          auto *terminator = BB.getTerminator();

          if (auto *ret = dyn_cast<llvm::ReturnInst>(terminator)) {
            // Get the returned value.
            auto *returned = ret->getReturnValue();

            // Propagate to the returned value.
            if (propagate(*returned, current_constraints)) {
              worklist.push_back(returned);
            }
          }
        }
      }
    }
  }

  return;
}

ConstraintInferenceDriver::ConstraintInferenceDriver(Constraints &constraints,
                                                     llvm::Module &M)
  : constraints(constraints),
    M(M) {
  // Collect initial constraints.
  this->init();

  // Infer constraints.
  this->infer();
}

// =======================
// Pass
Constraints ConstraintInference::run(llvm::Module &M,
                                     llvm::ModuleAnalysisManager &MAM) {

  // Initialize the result.
  Constraints result;

  // Run the constraint inference driver.
  ConstraintInferenceDriver driver(result, M);

  // Return the result.
  return result;
}

llvm::AnalysisKey ConstraintInference::Key;

} // namespace folio
