#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"

#include "memoir/ir/Instructions.hpp"
#include "memoir/ir/Types.hpp"
#include "memoir/utility/FunctionNames.hpp"

#include "folio/opportunities/Analysis.hpp"
#include "folio/opportunities/BitmapOpportunity.hpp"

#include "folio/analysis/Content.hpp"
#include "folio/analysis/ContentAnalysis.hpp"
#include "folio/analysis/ContentSimplification.hpp"

/*
using namespace llvm::memoir;

namespace folio {

namespace detail {

bool insert_into_range(vector<llvm::Value *> &range,
                       llvm::Value *from,
                       llvm::Value *to = nullptr) {

  // TODO: this needs to be split into an insertion and a verification function.

  // If the range is currently empty, just insert.
  if (range.empty()) {
    range.push_back(from);
    if (to) {
      range.push_back(to);
    }
  }

  // Try to find the insertion point for the new range.
  auto it = range.begin();
  for (; it != range.end(); ++it) {
    if (from == *it) {
      break;
    }
  }
  it = range.insert(it, from);

  // Insert the end of the range.
  if (to) {
    auto it2 = it;
    for (; it2 != range.end(); ++it2) {
      if (to == *it2) {
        break;
      }
    }

    range.insert(it2, to);
  }

  return true;
}

bool scalars_are_contiguous(
    vector<llvm::Value *> scalars,
    std::function<llvm::Loop *(llvm::Instruction &)> get_loop_for,
    std::function<llvm::ScalarEvolution &(llvm::Instruction &)> get_scev) {

  // We will try to construct a contiguous range of values from the vector.
  vector<llvm::Value *> range = {};
  for (auto *scalar : scalars) {
    // Handle constants.
    if (auto *constant_int = dyn_cast<llvm::ConstantInt>(scalar)) {
      // Check if the value falls within the range.
      if (not detail::insert_into_range(range, constant_int)) {
        return false;
      }
    }

    // Handle variables.
    else if (auto *inst = dyn_cast<llvm::Instruction>(scalar)) {

      // First, grab the SCEV analysis
      auto &SE = get_scev(*inst);

      // Then, get the parent loop.
      auto *loop = get_loop_for(*inst);
      if (not loop) {
        return false;
      }

      auto *iv = loop->getInductionVariable(SE);
      if (iv != inst) {
        return false;
      }

      auto loop_bounds = loop->getBounds(SE);
      if (not loop_bounds.has_value()) {
        return false;
      }

      // Check that the loop bounds are increasing
      // TODO: we can also check that this is decreasing by 1.
      if (loop_bounds->getDirection()
          != llvm::Loop::LoopBounds::Direction::Increasing) {
        return false;
      }

      // Ensure that the step value is 1.
      auto *step =
          dyn_cast_or_null<llvm::ConstantInt>(loop_bounds->getStepValue());
      if (not step) {
        return false;
      }
      auto step_value = step->getZExtValue();
      if (step_value != 1) {
        return false;
      }

      // Unpack the loop bounds.
      auto &from = loop_bounds->getInitialIVValue();
      auto &to = loop_bounds->getFinalIVValue();

      // See if the loop bounds fit within the current range.
      if (not detail::insert_into_range(range, &from, &to)) {
        return false;
      }
    }
  }

  // If we got this far, we succeeded.
  return true;
}

bool gather_scalars(UnionContent &C, vector<llvm::Value *> &scalars) {
  // Unpack the UnionContent.
  auto &lhs = C.lhs();
  auto &rhs = C.rhs();

  // Check the lhs.
  if (auto *lhs_scalar = dyn_cast<ScalarContent>(&lhs)) {
    scalars.push_back(&lhs_scalar->value());
  } else if (auto *lhs_union = dyn_cast<UnionContent>(&lhs)) {
    if (not gather_scalars(*lhs_union, scalars)) {
      // Propagate failures.
      return false;
    }
  } else {
    // Found a content that is neither a scalar nor something we can recurse on.
    return false;
  }

  // Check the rhs.
  if (auto *rhs_scalar = dyn_cast<ScalarContent>(&rhs)) {
    scalars.push_back(&rhs_scalar->value());
  } else if (auto *rhs_union = dyn_cast<UnionContent>(&rhs)) {
    if (not gather_scalars(*rhs_union, scalars)) {
      // Propagate failures.
      return false;
    }
  } else {
    // Found a content that is neither a scalar nor something we can recurse on.
    return false;
  }

  // If we got this far, we succeeded.
  return true;
}

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
          or isa<AssocWriteInst>(memoir_inst) or isa<RetPHIInst>(memoir_inst)
          or isa<UsePHIInst>(memoir_inst)) {
        gather_used_redefinitions(*user, used_redefinitions, visited);
      }
    }

    // Gather the value if it was used by an actual user.
    if (auto *has = into<AssocHasInst>(user)) {
      used_redefinitions.insert(use.get());
    } else if (auto *keys = into<AssocKeysInst>(user)) {
      used_redefinitions.insert(use.get());
    } else if (auto *read = into<AssocReadInst>(user)) {
      used_redefinitions.insert(use.get());
    }

    // Gather variable if folded on, or recurse on closed argument.
    if (auto *fold = into<FoldInst>(user)) {
      // If the variable use is the folded collection operand, gather.
      if (use == fold->getCollectionAsUse()) {
        used_redefinitions.insert(use.get());
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

bool included(Content &sub, UnionContent &super) {
  auto &lhs = super.lhs();
  if (auto *lhs_union = dyn_cast<UnionContent>(&lhs)) {
    return included(sub, *lhs_union);
  } else if (sub == lhs) {
    return true;
  }

  auto &rhs = super.rhs();
  if (auto *rhs_union = dyn_cast<UnionContent>(&rhs)) {
    return included(sub, *rhs_union);
  } else if (sub == rhs) {
    return true;
  }

  return false;
}

bool subset_of(Content &sub, Content &super) {
  // key(C) < C
  if (auto *sub_key = dyn_cast<KeyContent>(&sub)) {
    if (super == sub_key->collection()) {
      return true;
    }
  }

  // elem(C) < C
  if (auto *sub_elem = dyn_cast<ElementContent>(&sub)) {
    if (super == sub_elem->collection()) {
      return true;
    }
  }

  // C < C U D
  if (auto *super_union = dyn_cast<UnionContent>(&super)) {
    return included(sub, *super_union);
  }

  return false;
}

} // namespace detail

// =============================================================================
// Analysis
Opportunities BitmapOpportunityAnalysis::run(llvm::Module &M,
                                             llvm::ModuleAnalysisManager &MAM) {
  // Initialize an empty set of opportunities.
  Opportunities result;

  // Fetch the content analysis results.
  auto &contents =
      MEMOIR_SANITIZE(MAM.getCachedResult<ContentAnalysis>(M),
                      "Failed to get cached result of ContentAnalysis");

  // Construct a simplifier.
  // ContentSimplification simplifier(contents);

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
      // continue;
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
      println("  used redef ", *redef);

      // Fetch the content.
      auto found = contents.find(redef);
      if (found == contents.end()) {
        println("  no contents in analysis result for ");
        println("  ", *redef);
        domain = nullptr;
        break;
      }
      auto [redef_domain, _] = found->second;

      // Determine the maximal domain.

      // If the maximal domain is undefined, use this one.
      if (not domain) {
        domain = redef_domain;
        continue;
      }

      // If this domain is empty, no impact.
      if (isa<EmptyContent>(redef_domain)) {
        // Do nothing.
        continue;
      }

      // If the current domain is the same as this.
      if (*domain == *redef_domain) {
        // Do nothing.
        continue;
      }

      // If this domain is a subset of the current.
      if (detail::subset_of(*redef_domain, *domain)) {
        // Do nothing.
        continue;
      }

      // If the current domain is a subset of this.
      if (detail::subset_of(*domain, *redef_domain)) {
        domain = redef_domain;
        continue;
      }

      // Otherwise, we could not find a single, maximal domain.
      println("  could not find maximal domain!");
      println("    ", *domain);
      println("    ", *redef_domain);
      domain = nullptr;
      break;
    }

    // Ensure that we were able to find a single domain for all uses.
    if (not domain) {
      println("Failed to find a single domain for all uses.");
      continue;
    }

    // Simplify the contents for the given binding.
    // domain = &simplifier.simplify(*domain);

    // Now that we've found a domain, check if it satisfies our requirements.
    println("  passes applicability guard");
    println("  domain = ", *domain);
    println();

    // Construct a closure to access LLVM's loop analysis.
    // NOTE: for more complex induction variables we replace this with
    //   NOELLE's induction variable manager, but since we're solely looking
    //   for step variables this isn't needed at the moment.
    auto get_loop_for = [&](llvm::Instruction &I) -> llvm::Loop * {
      auto &BB =
          MEMOIR_SANITIZE(I.getParent(),
                          "Instruction is not attached to a basic block.");
      auto &F = MEMOIR_SANITIZE(I.getFunction(),
                                "Instruction is not attached to a function.");

      // Get the FunctionAnalysisManager proxy.
      auto &FAM = MAM.getResult<llvm::FunctionAnalysisManagerModuleProxy>(M)
                      .getManager();

      // Get LoopInfo from LLVM.
      auto loop_info = FAM.getCachedResult<llvm::LoopAnalysis>(F);

      return loop_info->getLoopFor(&BB);
    };

    // Construct a closure to access LLVM's SCEV analysis.
    auto get_scev = [&](llvm::Instruction &I) -> llvm::ScalarEvolution & {
      auto &F = MEMOIR_SANITIZE(I.getFunction(),
                                "Instruction is not attached to a function.");

      // Get the FunctionAnalysisManager proxy.
      auto &FAM = MAM.getResult<llvm::FunctionAnalysisManagerModuleProxy>(M)
                      .getManager();

      // Get LoopInfo from LLVM.
      return *FAM.getCachedResult<llvm::ScalarEvolutionAnalysis>(F);
    };

    // Unpack conditional contents.
    while (isa<ConditionalContent>(domain)) {
      auto *cond = cast<ConditionalContent>(domain);
      domain = &cond->content();
    }

    // The domain must be derived from a contiguous range.
    if (auto *scalar_content = dyn_cast<ScalarContent>(domain)) {

      // Check if the scalar is contiguous.
      vector<llvm::Value *> scalars = { &scalar_content->value() };
      if (detail::scalars_are_contiguous(scalars, get_loop_for, get_scev)) {
        println("  domain is contiguous!");
      }

    }
    // else if (auto *union_content = dyn_cast<UnionContent>(domain)) {
    //   // Gather all of the children, if they are ScalarContent.
    //   // TODO: this may need to be extended to include ConditionalContent.
    //   vector<llvm::Value *> scalars = {};
    //   if (detail::gather_scalars(*union_content, scalars)) {
    //     // Check if the set of scalars are contiguous.
    //     if (detail::scalars_are_contiguous(scalars, get_loop_for,
    //     get_scev)) {
    //       println("  domain is contiguous!");
    //     }
    //   }
    // }
    else {

      // See if a natural proxy exists.
      Proxy *proxy = nullptr;
      // TODO: auto *proxy = ProxyManager::has_natural_proxy(content, uses);
      if (not proxy) {

        // If no natural proxy exist, request an artificial one.
        println("Found a bitmap opportunity using an artificial proxy.");
        proxy = ProxyManager::request(*domain);
      }

      // If we could not find or create a proxy, continue.
      if (not proxy) {
        continue;
      }

      // Create the bitmap opportunity.
      auto *opportunity = new BitmapOpportunity(*alloc, proxy);

      result.push_back(opportunity);

      continue;
    }
  }

  // Coalesce bitmap opportunities to minimize redundancy.
  // TODO

  // Return the discovered opportunities.
  return result;
}

llvm::AnalysisKey BitmapOpportunityAnalysis::Key;
// =============================================================================

} // namespace folio
*/
