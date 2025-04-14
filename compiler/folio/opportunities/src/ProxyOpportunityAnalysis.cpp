// #include "folio/analysis/ContentAnalysis.hpp"
// #include "folio/analysis/ContentSimplification.hpp"

#include "folio/opportunities/Analysis.hpp"

using namespace llvm::memoir;

namespace folio {

namespace detail {

void gather_used_redefinitions(llvm::Value &V,
                               Set<llvm::Value *> &used_redefinitions,
                               Set<llvm::Value *> visited = {}) {

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
          or isa<UsePHIInst>(memoir_inst) or isa<ClearInst>(memoir_inst)) {
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

  // C = C
  if (sub == super) {
    return true;
  }

  // empty < C
  if (isa<EmptyContent>(&sub)) {
    return true;
  }

  // subset(C) < C
  if (auto *sub_subset = dyn_cast<SubsetContent>(&sub)) {
    return subset_of(sub_subset->content(), super);
  }

  // field(elem(C)) < field(C)
  if (auto *sub_field = dyn_cast<FieldContent>(&sub)) {
    Content *child = nullptr;
    if (auto *sub_subset = dyn_cast<SubsetContent>(&sub_field->parent())) {
      child = &sub_subset->content();
    }

    if (child) {
      return subset_of(
          Content::create<FieldContent>(*child, sub_field->field_index()),
          super);
    }
  }

  // cond(C) < C
  if (auto *sub_cond = dyn_cast<ConditionalContent>(&sub)) {
    return subset_of(sub_cond->content(), super);
  }

  // C < cond(C)
  if (auto *super_cond = dyn_cast<ConditionalContent>(&super)) {
    return subset_of(sub, super_cond->content());
  }

  // C < C U D
  if (auto *super_union = dyn_cast<UnionContent>(&super)) {
    return subset_of(sub, super_union->lhs())
           or subset_of(sub, super_union->rhs());
  }

  return false;
}

bool has_non_constant_scalar(Content &C) {
  // Returns true iff the content include a non-constant scalar.
  if (auto *scalar = dyn_cast<ScalarContent>(&C)) {
    if (not isa<llvm::Constant>(&scalar->value())) {
      return true;
    }
  } else if (auto *subset = dyn_cast<SubsetContent>(&C)) {
    return has_non_constant_scalar(subset->content());
  } else if (auto *cond = dyn_cast<ConditionalContent>(&C)) {
    return has_non_constant_scalar(cond->content());
  } else if (auto *field = dyn_cast<FieldContent>(&C)) {
    return has_non_constant_scalar(field->parent());
  } else if (auto *tuple = dyn_cast<TupleContent>(&C)) {
    bool has = false;
    for (auto *elem : tuple->elements()) {
      has |= has_non_constant_scalar(*elem);
    }
    return has;
  } else if (auto *union_content = dyn_cast<UnionContent>(&C)) {
    return has_non_constant_scalar(union_content->lhs())
           or has_non_constant_scalar(union_content->lhs());
  }

  return false;
}

Content &canonicalize(Content &input) {

  if (auto *cond = dyn_cast<ConditionalContent>(&input)) {
    // C | cond = C
    // TODO: detect cases where the condition can be summarized as a
    // comprehension
    return canonicalize(cond->content());

  } else if (auto *union_content = dyn_cast<UnionContent>(&input)) {
    // Recurse.
    auto &lhs = canonicalize(union_content->lhs());
    auto &rhs = canonicalize(union_content->rhs());

    if (&union_content->lhs() != &lhs or &union_content->rhs() != &rhs) {
      return Content::create<UnionContent>(lhs, rhs);
    }
  } else if (auto *subset = dyn_cast<SubsetContent>(&input)) {
    return canonicalize(subset->content());

  } else if (auto *field = dyn_cast<FieldContent>(&input)) {
    auto &content = canonicalize(field->parent());

    if (&field->parent() != &content) {
      return Content::create<FieldContent>(content, field->field_index());
    }
  }

  return input;
}

} // namespace detail

Opportunities ProxyOpportunityAnalysis::run(llvm::Module &M,
                                            llvm::ModuleAnalysisManager &MAM) {

  // The set of opportunities discovered by this analysis.
  // NOTE: opportunities must be allocated
  Opportunities result;

  // Fetch the Content Analysis results.
  // auto &contents = MAM.getResult<ContentAnalysis>(M);

  // Discover all proxy opportunities in the program.

  // For collection P to be a proxy of collection C:
  //  - \exists collection A, where domain(A) \subseteq domain(C)
  //  - P must be available at all uses of A
  //  - range(P) \contains domain(C)

  // Find all collection allocations in the program.
  Set<AllocInst *> allocations = {};
  auto *assoc_alloc_func =
      FunctionNames::get_memoir_function(M, MemOIR_Func::ALLOCATE);
  for (auto &uses : assoc_alloc_func->uses()) {
    auto *alloc = into<AllocInst>(uses.getUser());
    if (not alloc) {
      continue;
    }

    allocations.insert(alloc);
  }

#if 0
  auto *seq_alloc_func =
      FunctionNames::get_memoir_function(M, MemOIR_Func::ALLOCATE_SEQUENCE);
  for (auto &uses : seq_alloc_func->uses()) {
    auto *alloc_inst = into<SequenceAllocInst>(uses.getUser());
    if (not alloc_inst) {
      continue;
    }

    allocations.insert(alloc_inst);
  }
#endif

  // For each allocation that we found, determine a maximal domain for it, if
  // possible.
  // ContentSimplification simplifier(contents);
  Map<CollectionAllocInst *, Content *> domains = {};
  for (auto *alloc : allocations) {

    println("Analyzing ", *alloc);

    // Gather all used redefinitions of the allocation.
    Set<llvm::Value *> used_redefinitions = {};
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
      println("    domain = ", *redef_domain);

      // Determine the maximal domain.

      // Canonicalize the domain.
      auto &canonicalized = detail::canonicalize(*redef_domain);
      redef_domain = &canonicalized;

      // Simplify the domain.
      Content *simplified = redef_domain;
      for (auto iteration = 0; iteration < 10; ++iteration) {
        auto &next = simplifier.simplify(*simplified);
        if (&next == simplified) {
          break;
        }
        simplified = &next;
      }
      redef_domain = simplified;

      // If this domain is a SubsetContent, it is unhandled.
      // TODO: fix this.
      if (isa<KeysContent>(redef_domain)) {
        domain = nullptr;
        break;
      }

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

      if (not detail::has_non_constant_scalar(*redef_domain)) {
        domain = &Content::create<UnionContent>(*domain, *redef_domain);
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
      println();
      continue;
    }

    // TEMPORARY: Disable recursive proxies.
    // TODO: Make this more robust.
    if (auto *keys = dyn_cast<KeysContent>(domain)) {
      auto &domain_src = keys->collection();
      bool recursive = false;
      for (auto *redef : used_redefinitions) {
        if (&domain_src == redef) {
          warnln("Found a recursive content, currently unsupported.");
          recursive = true;
          break;
        }
      }
      if (recursive) {
        continue;
      }
    }

    if (detail::has_non_constant_scalar(*domain)) {
      warnln(
          "Found a scalar content to proxy, but this is currently unsupported.");
      println();
      domain = nullptr;
      break;
    }

    // Now that we've found a domain, check if it satisfies our requirements.
    println("  passes applicability guard");
    println("  max domain = ", *domain);
    println();

    // Record the allocation and its domain.
    domains[alloc] = domain;
  }

  // Coalesce equivalent domains to use the same pointer.
  OrderedSet<Content *> super_domains = {};
  for (auto it = domains.begin(); it != domains.end(); ++it) {
    auto *domain = it->second;

    // Check that we have not already seen this domain.
    if (super_domains.count(domain) > 0) {
      continue;
    } else {
      super_domains.insert(domain);
    }

    // Otherwise, set all other domains to this one.
    for (auto it2 = std::next(it); it2 != domains.end(); ++it2) {
      auto *other_domain = it2->second;

      // If these are the same Content objects, continue.
      if (domain == other_domain) {
        continue;
      }

      // If these are equivalent Content objects, replace with domain.
      if (*domain == *other_domain) {
        it2->second = domain;
        continue;
      }
    }
  }

  // TODO
  // Construct a mapping from subdomain to superdomain.
  Map<Content *, Content *> sub_domains = {};
  for (auto it = super_domains.begin(); it != super_domains.end(); ++it) {
    auto *domain = *it;

    // Find all other domains that are a subset of this one and insert them into
    // the mapping.
    for (auto it2 = std::next(it); it2 != super_domains.end(); ++it2) {
      auto *other_domain = *it2;

      // If we have already found this to be a subdomain, do not do so again.
      if (sub_domains.count(other_domain) > 0) {
        continue;
      }

      // Check if the other domain is a subset of this domain, and vice versa.
      if (detail::subset_of(*other_domain, *domain)) {
        sub_domains[other_domain] = domain;
      } else if (detail::subset_of(*domain, *other_domain)) {
        sub_domains[domain] = other_domain;
      }
    }
  }

  // Traverse the subdomain mapping to reify it.
  for (auto &[sub, super] : sub_domains) {

    while (sub_domains.count(super) > 0) {
      super = sub_domains[super];
    }
  }

  // Update the mapping from allocation to domain.
  Set<Content *> proxy_domains = {};
  for (auto &[alloc, domain] : domains) {
    auto found = sub_domains.find(domain);
    if (found != sub_domains.end()) {
      auto *new_domain = found->second;

      domain = new_domain;

      proxy_domains.insert(new_domain);
    } else {
      proxy_domains.insert(domain);
    }
  }

  // Construct a reverse mapping from domain to allocations.
  OrderedMultiMap<Content *, CollectionAllocInst *> domain_to_allocs = {};
  for (auto [alloc, domain] : domains) {
    println("Allocation ", *alloc);
    println("  uses proxy of ", *domain);
    insert_unique(domain_to_allocs, domain, alloc);
  }

  println("Found ", proxy_domains.size(), " proxy domains");
  for (auto *proxy_domain : proxy_domains) {
    println(*proxy_domain);
  }
  println();

  // For each super domain, fetch a proxy, if possible.
  OrderedMultiMap<Content *, Proxy *> proxies = {};
  for (auto *domain : proxy_domains) {

    // TODO: See if a natural proxy exists.
    if (Proxy *natural_proxy = nullptr) {
      insert_unique(proxies, domain, natural_proxy);
    }

    // Create an artificial proxy.
    auto *artificial_proxy = ProxyManager::request(*domain);
    insert_unique(proxies, domain, artificial_proxy);
  }

  // For each proxy, construct all possible opportunities its presents.
  for (auto [domain, proxy] : proxies) {
    auto range = domain_to_allocs.equal_range(domain);

    // Create the set of allocations that will be proxied.
    Set<CollectionAllocInst *> allocations = {};
    for (auto it = range.first; it != range.second; ++it) {
      // Get the allocation.
      auto *alloc = it->second;

      allocations.insert(alloc);
    }

    // TODO: Add an applicability guard here to ensure we can actually
    // construct the proxy and use it where needed. This will be some sort of
    // availability/dominance query.

    // Create a ProxyOpportunity for each alloc-proxy pair.
    auto *opportunity = new ProxyOpportunity(*proxy, allocations);

    // Create the proxy opportunity.
    result.push_back(opportunity);
  }

  // Return the discovered opportunities.
  return result;
}

llvm::AnalysisKey ProxyOpportunityAnalysis::Key;

} // namespace folio
