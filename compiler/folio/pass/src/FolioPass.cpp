#include "memoir/utility/Metadata.hpp"

#include "memoir/support/Casting.hpp"
#include "memoir/support/InternalDatatypes.hpp"
#include "memoir/support/Print.hpp"

#include "folio/analysis/ConstraintInference.hpp"
#include "folio/analysis/ContentAnalysis.hpp"
#include "folio/opportunities/Analysis.hpp"

#include "folio/transforms/SelectionMonomorphization.hpp"

#include "folio/solver/Implementation.hpp"
#include "folio/solver/Solver.hpp"

#include "folio/pass/Pass.hpp"

using namespace llvm::memoir;

namespace folio {

namespace detail {

void transform(llvm::Module &M,
               llvm::ModuleAnalysisManager &MAM,
               Candidate &candidate) {

  // First, annotate the selections while the LLVM Values are valid.
  for (const auto &[value, selection] : candidate.selections()) {
    // Unpack the selection.
    auto &impl = selection->implementation();

    println(*value, " --> ", impl.name());

    // Get the value as an instruction.
    auto *inst = dyn_cast<llvm::Instruction>(value);
    if (not inst) {
      warnln("Cannot annotate an argument with a selection.");
      continue;
    }

    // Attach the SelectionMetadata to the instruction.
    auto metadata = Metadata::get_or_add<SelectionMetadata>(*inst);

    // Set the implementation to that selected.
    metadata.setImplementation(impl.name());
  }

  // Closure to fetch the Selection for a given value.
  auto get_selection = [&](llvm::Value &V) -> Selection & {
    auto &selections = candidate.selections();
    auto found = selections.find(&V);
    MEMOIR_ASSERT(found != selections.end(),
                  "Could not find selection for given value!");
    return MEMOIR_SANITIZE(found->second, "Selection is NULL!");
  };

  // Then, transform the program to exploit all opportunities.
  for (auto *opportunity : candidate.opportunities()) {
    opportunity->exploit(get_selection, MAM);
  }

  // Finally, perform selection monomorphization.
  SelectionMonomorphization monomorph(M);

  return;
}

} // namespace detail

llvm::PreservedAnalyses FolioPass::run(llvm::Module &M,
                                       llvm::ModuleAnalysisManager &MAM) {

  // Fetch the ConstraintInference results.
  auto &constraints = MAM.getResult<ConstraintInference>(M);

  // Collect all of the selectable variables.
  set<llvm::Value *> selectable = {};
  for (auto &F : M) {
    if (F.empty()) {
      continue;
    }

    for (auto &BB : F) {
      for (auto &I : BB) {
        auto *memoir_inst = into<MemOIRInst>(&I);
        if (not memoir_inst) {
          continue;
        }

        // For the time being, we will only consider explicit allocations as
        // selectable.
        if (auto *seq = dyn_cast<SequenceAllocInst>(memoir_inst)) {
          selectable.insert(&I);
        } else if (auto *assoc = dyn_cast<AssocAllocInst>(memoir_inst)) {
          selectable.insert(&I);
        }
      }
    }
  }

  // Fetch the OpportunityDiscovery results.
  auto &opportunities = MAM.getResult<OpportunityAnalysis>(M);

  // Instantiate all available implementations.
  Implementations implementations = {
    { "stl_vector",
      SeqImplementation("stl_vector", { PointerStableConstraint() }) },
    // { "stl_list", SeqImplementation("stl_list", {}) },
    { "stl_unordered_map",
      AssocImplementation("stl_unordered_map",
                          { OperationConstraint<ReverseFoldInst>() }) },
    { "stl_map", AssocImplementation("stl_map", {}) },
    { "stl_unordered_set",
      SetImplementation("stl_unordered_set",
                        { OperationConstraint<ReverseFoldInst>() }) },
  };

  // Pass the analysis results to the solver.
  Solver solver(M, selectable, constraints, opportunities, implementations);

  // If there are no candidates, we're all done.
  if (solver.candidates().empty()) {
    infoln("No candidates generated.");
    return llvm::PreservedAnalyses::all();
  }

  // DEBUG: print the list of candidates.
  auto candidate_index = 0;
  for (auto &candidate : solver.candidates()) {
    debugln("Candidate ", std::to_string(candidate_index++));
    for (auto [def, selection] : candidate.selections()) {
      debugln("  ", value_name(*def));
      debugln(selection->type());
      debugln("  implementation=", selection->implementation().name());
    }
    auto num_exploited = candidate.opportunities().size();
    debugln("  ", std::to_string(num_exploited), " opportunities exploited.");
    debugln();
  }

  // Select a candidate.
  auto &selected_candidate = solver.candidates().front();

  // Transform the program to utilize the selected candidate.
  detail::transform(M, MAM, selected_candidate);

  // All done.
  return llvm::PreservedAnalyses::none();
}

} // namespace folio
