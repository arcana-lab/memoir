#include "memoir/ir/Instructions.hpp"
#include "memoir/ir/Types.hpp"
#include "memoir/utility/FunctionNames.hpp"

#include "folio/opportunities/Analysis.hpp"
#include "folio/opportunities/BitmapOpportunity.hpp"

#include "folio/analysis/Content.hpp"
#include "folio/analysis/ContentAnalysis.hpp"

using namespace llvm::memoir;

namespace folio {

namespace detail {

void gather_used_redefinitions(llvm::Value &V,
                               set<llvm::Value *> &used_redefinitions,
                               set<llvm::Value *> visited = {}) {

  if (visited.count(&V) > 0) {
    return;
  } else {
    visited.insert(&V);
  }

  for (auto &use : V.uses()) {
    auto *user = use.getUser();

    // Recurse on redefinitions.
    if (auto *phi = dyn_cast<llvm::PHINode>(user)) {
      gather_used_redefinitions(*user, used_redefinitions, visited);
    } else if (auto *memoir_inst = into<MemOIRInst>(user)) {
      if (isa<AssocInsertInst>(memoir_inst) or isa<AssocRemoveInst>(memoir_inst)
          or isa<RetPHIInst>(memoir_inst) or isa<UsePHIInst>(memoir_inst)) {
        gather_used_redefinitions(*user, used_redefinitions, visited);
      }
    }

    // Gather the value if it was used by an actual user.
    if (auto *has = into<AssocHasInst>(user)) {
      used_redefinitions.insert(user);
    } else if (auto *keys = into<AssocKeysInst>(user)) {
      used_redefinitions.insert(user);
    }

    // Gather variable if folded on, or recurse on closed argument.
    if (auto *fold = into<FoldInst>(user)) {
      // If the variable use is the folded collection operand, gather.
      if (use == fold->getCollectionAsUse()) {
        used_redefinitions.insert(user);
        continue;
      }

      // Otherwise, recurse on the argument.
      if (use == fold->getInitialAsUse()) {
        // Gather uses of the accumulator argument.
        gather_used_redefinitions(fold->getAccumulatorArgument(),
                                  used_redefinitions,
                                  visited);

        // Gather uses of the resultant.
        gather_used_redefinitions(fold->getResult(),
                                  used_redefinitions,
                                  visited);
      } else {
        gather_used_redefinitions(fold->getClosedArgument(use),
                                  used_redefinitions,
                                  visited);
      }
    }
  }

  return;
}

} // namespace detail

// =============================================================================
// Analysis
Opportunities BitmapOpportunityAnalysis::run(llvm::Module &M,
                                             llvm::ModuleAnalysisManager &MAM) {
  // Initialize an empty set of opportunities.
  Opportunities result;

  // Fetch the content analysis results.
  auto contents = MAM.getResult<ContentAnalysis>(M);

  // For a given collection A to be transformed into a bitmapped collection:
  //  1. A : Assoc<Int,void>
  //  2. \exists B | domain(A) \subseteq domain(B), domain(B) is contiguous
  //              or domain(A) \subseteq range(B),  range(B)  is contiguous

  // Find all set allocations in the program.
  set<AssocAllocInst *> set_allocations = {};
  auto *assoc_alloc_func =
      FunctionNames::get_memoir_function(M, MemOIR_Func::ALLOCATE_ASSOC_ARRAY);
  for (auto &uses : assoc_alloc_func->uses()) {
    // Collection must be associative.
    auto *alloc_inst = into<AssocAllocInst>(uses.getUser());
    if (not alloc_inst) {
      continue;
    }

    // Value must be void.
    auto &value_type = alloc_inst->getValueType();
    if (not isa<VoidType>(&value_type)) {
      continue;
    }

    // Key must be an integer.
    // TODO: this can be relaxed to any hashable type.
    auto &key_type = alloc_inst->getKeyType();
    if (not isa<IntegerType>(&key_type)) {
      continue;
    }

    set_allocations.insert(alloc_inst);
  }

  // For each set allocation that we found.
  for (auto *alloc : set_allocations) {

    println("Analyzing ", *alloc);

    // Gather all used redefinitions of the allocation.
    set<llvm::Value *> used_redefinitions = {};

    detail::gather_used_redefinitions(alloc->getCallInst(), used_redefinitions);

    // Determine if all uses share a content.
    Content *domain = nullptr;
    for (auto *redef : used_redefinitions) {
      // Fetch the content.
      auto found = contents.find(redef);
      if (found == contents.end()) {
        domain = nullptr;
        break;
      }
      auto [redef_domain, _] = found->second;

      // Determine the maximal domain.
      if (not domain) {
        domain = redef_domain;
      }

      // Merge the redef domain with the current domain.
      if (not isa<EmptyContent>(redef_domain)) {

        // TODO: replace this check with equivalence (if needed)
        if (*domain != *redef_domain) {
          println("  domains not equivalent!");
          println("    ", *domain);
          println("    ", *redef_domain);
          domain = nullptr;
          break;
        }

        // If the redef domain is "better" than the current one, replace it.
        if (isa<EmptyContent>(domain)) {
          domain = redef_domain;
        }
      }
    }

    // Ensure that we were able to find a single domain for all uses.
    if (not domain) {
      println("Failed to find a single domain for all uses.");
      continue;
    }

    // Now that we've found a domain, check if it satisfies our requirements.
    println("  passes applicability guard");
    println("  domain = ", *domain);
    println();

    // The domain must be derived from a contiguous range.
  }

  // Return the discovered opportunities.
  return result;
}

llvm::AnalysisKey BitmapOpportunityAnalysis::Key;
// =============================================================================

} // namespace folio
