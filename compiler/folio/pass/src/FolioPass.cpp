#include "memoir/utility/Metadata.hpp"

#include "memoir/support/Casting.hpp"
#include "memoir/support/InternalDatatypes.hpp"
#include "memoir/support/Print.hpp"

#include "folio/analysis/ConstraintInference.hpp"
#include "folio/analysis/OpportunityDiscovery.hpp"

#include "folio/transforms/SelectionMonomorphization.hpp"

#include "folio/solver/Implementation.hpp"
#include "folio/solver/Solver.hpp"

#include "folio/pass/Pass.hpp"

using namespace llvm::memoir;

namespace folio {

namespace detail {

void transform(llvm::Module &M, const Candidate &candidate) {

  // First, annotate the selections while the LLVM Values are valid.
  for (const auto &[value, impl] : candidate.selections()) {
    println(*value, " --> ", impl->name());

    // Get the value as an instruction.
    auto *inst = dyn_cast<llvm::Instruction>(value);
    if (not inst) {
      warnln("Cannot annotate an argument with a selection.");
      continue;
    }

    // Attach the SelectionMetadata to the instruction.
    auto metadata = Metadata::get_or_add<SelectionMetadata>(*inst);

    // Set the implementation to that selected.
    metadata.setImplementation(impl->name());
  }

  // Then, transform the program to exploit all opportunities.
  // TODO

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
  // auto &opportunities = MAM.getResult<OpportunityDiscovery>(M);
  Opportunities opportunities;

  // Instantiate all available implementations.
  Implementations implementations = {
    { "stl_vector",
      SeqImplementation("stl_vector", { PointerStableConstraint() }) },
    // { "stl_list", SeqImplementation("stl_list", {}) },
    { "stl_unordered_map",
      AssocImplementation("stl_unordered_map", { PointerStableConstraint() }) },
    { "stl_map", AssocImplementation("stl_map", {}) },
    { "stl_unordered_set", SetImplementation("stl_unordered_set", {}) },
  };

  // Pass the analysis results to the solver.
  Solver solver(selectable, constraints, opportunities, implementations);

  // If there are no candidates, we're all done.
  if (solver.candidates().empty()) {
    println("Empty candidate");
    return llvm::PreservedAnalyses::all();
  }

  // Select a candidate.
  auto &selected_candidate = solver.candidates().front();

  // Transform the program to utilize the selected candidate.
  detail::transform(M, selected_candidate);

  // All done.
  return llvm::PreservedAnalyses::none();
}

} // namespace folio
