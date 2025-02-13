// LLVM
#include "llvm/IR/Instructions.h"
#include "llvm/Transforms/Utils/Cloning.h"

// MEMOIR
#include "memoir/ir/Instructions.hpp"

#include "memoir/support/Casting.hpp"
#include "memoir/support/InternalDatatypes.hpp"
#include "memoir/support/Print.hpp"

#include "memoir/utility/Metadata.hpp"

// Folio
#include "folio/transforms/SelectionMonomorphization.hpp"

using namespace llvm::memoir;

namespace folio {

using ImplList = vector<std::optional<std::string>>;
struct Selections {

  using ID = uint32_t;

  // A mapping from implementation list to its unique identifier.
  ordered_map<ImplList, ID> implementations;

  // A multimapping from value to its implementations.
  ordered_multimap<llvm::Value *, ID> selections;

  // Returns TRUE if the value changed.
  bool insert(llvm::Value &V, ID id) {
    auto old_size = this->selections.size();
    insert_unique(this->selections, &V, id);
    return old_size != this->selections.size();
  }

  bool insert(llvm::Value &V, const ImplList &selection) {
    ID id = this->implementations.size();
    auto found = this->implementations.find(selection);
    if (found != this->implementations.end()) {
      id = found->second;
    } else {
      this->implementations[selection] = id;
    }

    return this->insert(V, id);
  }

  bool propagate(llvm::Value &from, llvm::Value &to) {
    // Check for trivial cycles.
    if (&from == &to) {
      return false;
    }

    // Gather all of the selections of @from.
    vector<ID> from_selections;
    from_selections.reserve(this->selections.count(&from));
    for (auto it = this->selections.lower_bound(&from);
         it != this->selections.upper_bound(&from);
         ++it) {
      from_selections.push_back(it->second);
    }

    // Union the selections.
    bool changed = false;
    for (const auto &selection : from_selections) {
      changed |= this->insert(to, selection);
    }

    return changed;
  }

  const ImplList &from_id(const ID &id) {
    for (const auto &[sel, sel_id] : this->implementations) {
      if (id == sel_id) {
        return sel;
      }
    }
    MEMOIR_UNREACHABLE("Failed to find selection with given ID!");
  }

  size_t size() {
    return this->selections.size();
  }

  decltype(auto) begin() const {
    return this->selections.begin();
  }

  decltype(auto) end() const {
    return this->selections.end();
  }

  decltype(auto) count(llvm::Value *value) const {
    return this->selections.count(value);
  }

  decltype(auto) find(llvm::Value *value) const {
    return this->selections.find(value);
  }

  decltype(auto) clear() {
    return this->selections.clear();
  }
};

namespace detail {

void collect_declarations(Selections &selections, llvm::Module &M) {

  for (auto &F : M) {
    if (F.empty()) {
      continue;
    }

    for (auto &BB : F) {
      for (auto &I : BB) {
        auto *memoir_inst = into<MemOIRInst>(&I);

        // Skip non-memoir instructions.
        if (not memoir_inst) {
          continue;
        }

        // Check if the instruction has a selection attached to it.
        auto metadata = Metadata::get<SelectionMetadata>(I);
        if (not metadata.has_value()) {
          continue;
        }

        // Store the selection for this declaration.
        ImplList selection = {};
        for (auto impl : metadata->implementations()) {
          if (impl.has_value()) {
            selection.push_back(impl);
          } else {
            selection.push_back({});
          }
        }

        selections.insert(I, selection);
      }
    }
  }

  return;
}

void propagate(Selections &selections,
               vector<llvm::Value *> &worklist,
               llvm::Module &M,
               llvm::Value &from,
               llvm::Use &use) {
  // Unpack the use.
  auto *user = use.getUser();
  auto operand_no = use.getOperandNo();

  // Ensure that the user is not NULL.
  if (not user) {
    return;
  }

  // Handle each use case in turn.
  if (auto *memoir_inst = into<MemOIRInst>(user)) {

    if (auto *fold = dyn_cast<FoldInst>(memoir_inst)) {

      // If the use is the collection being folded over.
      if (&fold->getObjectAsUse() == &use) {

        // Propagate to the instruction, don't recurse.
        selections.propagate(from, fold->getResult());

        // If the element type of the fold is a collection, propagate to the
        // corresponding argument.
        if (Type::is_unsized(fold->getElementType())) {
          if (auto *argument = fold->getElementArgument()) {
            selections.propagate(from, *argument);
          }
        }

      } else if (&fold->getInitialAsUse() == &use) {
        // If the use is the initial value:

        // Propagate to uses of the resultant.
        auto &result = fold->getResult();
        for (auto &result_use : result.uses()) {
          detail::propagate(selections, worklist, M, from, result_use);
        }

        // Propagate to the corresponding argument.
        auto &argument = fold->getAccumulatorArgument();
        if (selections.propagate(from, argument)) {
          worklist.push_back(&argument);
        }

      } else if (auto *argument = fold->getClosedArgument(use)) {
        // Otherwise, the collection is closed on, propagate to the
        // corresponding argument, but don't recurse.
        if (selections.propagate(from, *argument)) {
          worklist.push_back(argument);
        }
      }

    } else if (auto *update = dyn_cast<UpdateInst>(memoir_inst)) {

      // Ensure that the use is the object being updated.
      if (&use == &update->getObjectAsUse()) {
        // If the memoir operation is a partial redefinition of the input
        // collection, propagate.
        if (selections.propagate(from, *user)) {
          worklist.push_back(user);
        }
      }

    } else if (isa<UsePHIInst>(memoir_inst) or isa<RetPHIInst>(memoir_inst)
               or isa<CopyInst>(memoir_inst)) {

      // If the memoir operation is a partial redefinition of the input
      // collection, propagate.
      if (selections.propagate(from, *user)) {
        worklist.push_back(user);
      }

    } else if (isa<AccessInst>(memoir_inst) or isa<DeleteInst>(memoir_inst)) {

      // If the memoir operation is an access to the input collection,
      // propagate, but don't recurse.
      selections.propagate(from, *user);
    }

  } else if (auto *phi = dyn_cast<llvm::PHINode>(user)) {

    // Propagate to the PHI.
    if (selections.propagate(from, *phi)) {
      worklist.push_back(phi);
    }

  } else if (auto *extract = dyn_cast<llvm::ExtractValueInst>(user)) {
    // Propagate to the extracted value.
    if (selections.propagate(from, *extract)) {
      worklist.push_back(extract);
    }

  } else if (auto *call = dyn_cast<llvm::CallBase>(user)) {
    // Propagate to _all_ possible callee arguments.

    // If this is a direct call, propagate to it.
    if (auto *called_function = call->getCalledFunction()) {
      auto &argument = MEMOIR_SANITIZE(called_function->getArg(operand_no),
                                       "Argument out of range.");
      if (selections.propagate(from, argument)) {
        worklist.push_back(&argument);
      }
    }

    // Otherwise, find all possible callees and propagate to them.
    else {
      auto *function_type = call->getFunctionType();

      // For each function that shares this function type.
      for (auto &F : M) {
        if (F.empty()) {
          continue;
        }

        if (F.getFunctionType() == function_type) {
          auto &argument =
              MEMOIR_SANITIZE(F.getArg(operand_no), "Argument out of range.");
          if (selections.propagate(from, argument)) {
            worklist.push_back(&argument);
          }
        }
      }
    }
  }
}

void propagate_declarations(Selections &selections, llvm::Module &M) {

  // Now, we need to find any memoir instruction that _would_ have multiple
  // selections.
  // We will accomplish this with a simple forward data flow analysis,
  // propagating all selections to their redefinitions.

  // Initialize the worklist.
  vector<llvm::Value *> worklist = {};
  worklist.reserve(selections.size());
  for (const auto &[value, selection] : selections) {
    worklist.push_back(value);
  }

  // Run the data flow analysis until a fixed-point is reached.
  while (not worklist.empty()) {
    // Pop a value off of the worklist.
    auto *current = worklist.back();
    worklist.pop_back();

    // For each use of the current value:
    for (auto &use : current->uses()) {
      detail::propagate(selections, worklist, M, *current, use);
    }
  }

  return;
}

bool transform(const Selections &selections) {
  // Track whether or not we have transformed the function.
  bool transformed = false;

  // Collect the polymorphic values (those that have multiple selections).
  set<llvm::Value *> polymorphic = {};
  for (const auto &[value, selection] : selections) {

    // If the value has more than one selection, it is polymorphic.
    if (selections.count(value) > 1) {
      polymorphic.insert(value);
    }
  }

  // If there are no polymorphic values, return.
  if (polymorphic.empty()) {
    return false;
  }

  // First, handle any polymorphic argument, where we need to monomorphize the
  // function for a given call site.
  for (auto *value : polymorphic) {
    if (auto *arg = dyn_cast<llvm::Argument>(value)) {
      // If an argument is polymorphic, fetch all of the possible call sites and
      // partition them based on their argument's selection, if it is
      // monomorphic.
      map<llvm::Use *, Selections::ID> caller_to_selection = {};

      // Fetch the parent function.
      auto *function = arg->getParent();

      // For each possible callee.
      for (auto &use : function->uses()) {
        auto *user = use.getUser();
        auto *call = dyn_cast<llvm::CallBase>(user);
        if (not call) {
          warnln("MEMOIR function ", function->getName(), " has non-call use.");
          continue;
        }

        // Check that the function is the called operand.
        if (call->getCalledOperand() != function) {
          // If the call is neither a fold nor a ret PHI, warn the user.
          if (not into<FoldInst>(call) or not into<RetPHIInst>(call)) {
            warnln("MEMOIR function ",
                   function->getName(),
                   " may be indirectly called.");
            continue;
          }
        }

        // Get the corresponding operand.
        auto *operand = call->getArgOperand(arg->getArgNo());

        // If the operand is monomorphic, we will save it.
        if (selections.count(operand) == 1) {
          caller_to_selection[&use] = selections.find(operand)->second;
        }
      }

      // Partition the callers by their selections.
      ordered_map<Selections::ID, vector<llvm::Use *>>
          selection_to_callers = {};
      for (const auto &[caller, selection] : caller_to_selection) {
        selection_to_callers[selection].push_back(caller);
      }

      // For each partition, create a unique function for that set of callers.
      for (const auto &[selection, callers] : selection_to_callers) {
        // Create a unique clone of the function for these callers.
        llvm::ValueToValueMapTy vmap;
        auto *clone = llvm::CloneFunction(function, vmap);

        // For each of the callers, replace the called function with this unique
        // clone.
        for (auto *caller : callers) {
          // Replace the use.
          caller->set(clone);

          // Update any RetPHIs that follow.
          auto *next =
              dyn_cast<llvm::Instruction>(caller->getUser())->getNextNode();
          while (next) {
            if (auto *ret_phi = into<RetPHIInst>(next)) {
              ret_phi->getCalledOperandAsUse().set(clone);
            } else if (isa<llvm::CallBase>(next)) {
              break;
            }

            next = next->getNextNode();
          }
        }

        // Mark the module as transformed.
        transformed = true;

        // We could improve the logic of this pass to keep track of changes and
        // update the selection results, but for the time being we will just do
        // the simple thing and apply one transformation at a time.
        return true;
      }
    }
  }

  return transformed;
}

void annotate(const map<MemOIRInst *, const ImplList *> &selections) {

  // For each value with a selection.
  for (const auto &[memoir_inst, selection] : selections) {

    // Annotate the instruction with the selection.
    auto metadata =
        Metadata::get_or_add<SelectionMetadata>(memoir_inst->getCallInst());

    unsigned selection_index = 0;
    for (const auto &impl : *selection) {
      if (impl.has_value()) {
        metadata.setImplementation(impl.value(), selection_index);
      } else {
        // TODO: if we don't have a value, get the default implementation.
      }

      // TODO: skip along if our implementation tiles multiple dimensions.,
      ++selection_index;
    }
  }

  return;
}

} // namespace detail

SelectionMonomorphization::SelectionMonomorphization(llvm::Module &M) : M(M) {
  // Initialize an empty mapping for selections.
  Selections selections = {};

  do {
    // Clear the last round of selection that we had.
    selections.clear();

    // Collect all collection declarations and their selections.
    detail::collect_declarations(selections, M);

    // Propagate the declaration selections to all users.
    detail::propagate_declarations(selections, M);

    // Transform the program to ensure that each selection is monomorphized.
    // Continue until we don't transform the program.
  } while (detail::transform(selections));

  // Validate that the program is monomorphized.
  map<MemOIRInst *, const ImplList *> selections_to_annotate = {};
  for (const auto &[value, selection] : selections) {
    // Only insert mappings for memoir instructions.
    auto *memoir_inst = into<MemOIRInst>(value);
    if (not memoir_inst) {
      continue;
    }

    // If the instruction has a polymorphic selection, error!
    if (selections.count(value) > 1) {
      MEMOIR_UNREACHABLE("Failed to monomorphize selections in the program!");
    }

    // Set the selection for the instruction.
    selections_to_annotate[memoir_inst] = &selections.from_id(selection);
  }

  // Annotate instructions with their selection.
  detail::annotate(selections_to_annotate);
}

} // namespace folio
