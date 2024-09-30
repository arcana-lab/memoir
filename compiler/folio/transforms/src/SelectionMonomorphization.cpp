// LLVM
#include "llvm/IR/Instructions.h"

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

namespace detail {

void collect_declarations(
    ordered_multimap<llvm::Value *, std::string> &selections,
    llvm::Module &M) {

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

        // Fetch the selected implementation from the metadata.
        auto selection = metadata->getImplementation();

        // Store the selection for this declaration.
        insert_unique(selections, (llvm::Value *)&I, selection);
      }
    }
  }

  return;
}

bool propagate(ordered_multimap<llvm::Value *, std::string> &selections,
               llvm::Value &from,
               llvm::Value &to) {

  // Gather all of the selections of @from.
  vector<std::string> from_selections;
  from_selections.reserve(selections.count(&from));
  for (auto it = selections.lower_bound(&from);
       it != selections.upper_bound(&from);
       ++it) {
    from_selections.push_back(it->second);
  }

  // Save the original size of the collection so that we can check if any were
  // inserted later.
  auto original_size = selections.count(&to);

  // Union the selections.
  for (auto selection : from_selections) {
    insert_unique(selections, &to, selection);
  }

  // If the selections of @to have been modified, add it to the worklist.
  auto new_size = selections.count(&to);
  auto changed = (original_size != new_size);

  return changed;
}

void propagate(ordered_multimap<llvm::Value *, std::string> &selections,
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

    if (auto *insert_seq = dyn_cast<SeqInsertSeqInst>(memoir_inst)) {

      // If the use is the base collection, propagate.
      if (insert_seq->getBaseCollectionAsUse().getOperandNo() == operand_no) {
        if (detail::propagate(selections, from, *user)) {
          worklist.push_back(user);
        }
      }

    } else if (auto *fold = dyn_cast<FoldInst>(memoir_inst)) {

      if (&fold->getCollectionAsUse() == &use) {
        // If the use is the collection being folded over.

        // Propagate to the instruction, don't recurse.
        detail::propagate(selections, from, fold->getResult());

      } else if (&fold->getInitialAsUse() == &use) {
        // If the use is the initial value:

        // Propagate to uses of the resultant.
        auto &result = fold->getResult();
        for (auto &result_use : result.uses()) {
          detail::propagate(selections, worklist, M, from, result_use);
        }

        // Propagate to the corresponding argument.
        auto &argument = fold->getAccumulatorArgument();
        if (detail::propagate(selections, from, argument)) {
          worklist.push_back(&argument);
        }

      } else {
        // Otherwise, the collection is closed on, propagate to the
        // corresponding argument, but don't recurse.
        auto &argument = fold->getClosedArgument(use);
        if (detail::propagate(selections, from, argument)) {
          worklist.push_back(&argument);
        }
      }

    } else if (isa<WriteInst>(memoir_inst) or isa<InsertInst>(memoir_inst)
               or isa<RemoveInst>(memoir_inst) or isa<SwapInst>(memoir_inst)
               or isa<UsePHIInst>(memoir_inst) or isa<DefPHIInst>(memoir_inst)
               or isa<RetPHIInst>(memoir_inst) or isa<CopyInst>(memoir_inst)) {
      // If the memoir operation is a partial redefinition of the input
      // collection, propagate.
      if (detail::propagate(selections, from, *user)) {
        worklist.push_back(user);
      }

    } else if (isa<AccessInst>(memoir_inst) or isa<AssocHasInst>(memoir_inst)
               or isa<AssocKeysInst>(memoir_inst)
               or isa<DeleteCollectionInst>(memoir_inst)
               or isa<SizeInst>(memoir_inst)) {
      // If the memoir operation is an access to the input collection,
      // propagate, but don't recurse.
      detail::propagate(selections, from, *user);
    }

  } else if (auto *phi = dyn_cast<llvm::PHINode>(user)) {
    // Propagate to the PHI.
    if (detail::propagate(selections, from, *phi)) {
      worklist.push_back(phi);
    }

  } else if (auto *extract = dyn_cast<llvm::ExtractValueInst>(user)) {
    // Propagate to the extracted value.
    if (detail::propagate(selections, from, *extract)) {
      worklist.push_back(extract);
    }

  } else if (auto *call = dyn_cast<llvm::CallBase>(user)) {
    // Propagate to _all_ possible callee arguments.

    // If this is a direct call, propagate to it.
    if (auto *called_function = call->getCalledFunction()) {
      auto &argument = MEMOIR_SANITIZE(called_function->getArg(operand_no),
                                       "Argument out of range.");
      if (detail::propagate(selections, from, argument)) {
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
          if (detail::propagate(selections, from, argument)) {
            worklist.push_back(&argument);
          }
        }
      }
    }
  }
}

void propagate_declarations(
    ordered_multimap<llvm::Value *, std::string> &selections,
    llvm::Module &M) {

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

void annotate(const ordered_multimap<llvm::Value *, std::string> &selections) {

  // For each value with a selection.
  for (const auto &[value, selection] : selections) {

    // Ensure that the value is a memoir instruction, other instructions can be
    // hylomorphic.
    auto *memoir_inst = into<MemOIRInst>(value);
    if (not memoir_inst) {
      continue;
    }

    // Check that this instruction has _only one_ selection, if it has more then
    // we failed to monomorphize the program!
    if (selections.count(value) > 1) {
      warnln(*value, " has a hylomorphic selection!");
      MEMOIR_UNREACHABLE("Failed to monomorphize the program!");
    }

    // Annotate the instruction with the selection.
    auto metadata =
        Metadata::get_or_add<SelectionMetadata>(memoir_inst->getCallInst());

    metadata.setImplementation(selection);
  }

  return;
}

} // namespace detail

SelectionMonomorphization::SelectionMonomorphization(llvm::Module &M) : M(M) {
  // Initialize an empty mapping for selections.
  ordered_multimap<llvm::Value *, std::string> selections = {};

  // Collect all collection declarations and their selections.
  detail::collect_declarations(selections, M);

  // Propagate the declaration selections to all users.
  detail::propagate_declarations(selections, M);

  // Transform the program to ensure that each selection is monomorphized.
  // TODO

  // Annotate instructions with their selection.
  detail::annotate(selections);
}

} // namespace folio
