#include "folio/analysis/ContentSimplification.hpp"

using namespace llvm::memoir;

namespace folio {

Content &ContentSimplification::simplify(Content &C) {
  return this->visit(C);
}

// Helper functions to lookup contents in the environment.
Content *ContentSimplification::lookup_domain(llvm::Value &V) {
  // Lookup the content.
  auto found = this->contents.find(&V);
  if (found == this->contents.end()) {
    return nullptr;
  }

  auto [domain, _] = found->second;

  // The result must be in a simplified form.
  if (isa<TupleContent>(domain) or isa<EmptyContent>(domain)
      or isa<KeysContent>(domain)) {
    return domain;
  }

  return nullptr;
}

Content *ContentSimplification::lookup_range(llvm::Value &V) {
  // Lookup the content.
  auto found = this->contents.find(&V);
  if (found == this->contents.end()) {
    return nullptr;
  }

  auto [_, range] = found->second;

  // The result must be in a simplified form.
  if (isa<TupleContent>(range) or isa<EmptyContent>(range)
      or isa<ElementsContent>(range)) {
    return range;
  }

  return nullptr;
}

Content &ContentSimplification::visitContent(Content &C) {
  return C;
}

Content &ContentSimplification::visitConditionalContent(ConditionalContent &C) {
  // Recurse.
  auto &content = this->visit(C.content());

  // empty | c == empty
  if (isa<EmptyContent>(&content)) {
    return content;
  }

  // If recursion simplified the inner content, update ourselves.
  if (&content != &C.content()) {
    return Content::create<ConditionalContent>(content,
                                               C.predicate(),
                                               C.lhs(),
                                               C.rhs());
  }

  return C;
}

Content &ContentSimplification::visitKeysContent(KeysContent &C) {

  if (auto *lookup = this->lookup_domain(C.collection())) {
    return *lookup;
  }

  return C;
}

Content &ContentSimplification::visitElementsContent(ElementsContent &C) {

  if (auto *lookup = this->lookup_range(C.collection())) {
    return *lookup;
  }

  return C;
}

namespace detail {
Content &dedup(UnionContent &C, vector<Content *> &seen) {

  auto *lhs = &C.lhs();
  auto *rhs = &C.rhs();

  if (auto *lhs_union = dyn_cast<UnionContent>(lhs)) {
    // Recurse on nested unions.
    lhs = &dedup(*lhs_union, seen);
  } else {
    // Check if this is a duplicate.
    bool duplicate = false;
    for (auto *other : seen) {
      if (*lhs == *other) {
        duplicate = true;
      }
    }

    // If we found a duplicate, remove it.
    if (duplicate) {
      lhs = nullptr;
    } else {
      // Mark lhs as seen.
      seen.push_back(lhs);
    }
  }

  if (auto *rhs_union = dyn_cast<UnionContent>(rhs)) {
    // Recurse on nested unions.
    rhs = &dedup(*rhs_union, seen);
  } else {
    // Check if this is a duplicate.
    bool duplicate = false;
    for (auto *other : seen) {
      if (*rhs == *other) {
        duplicate = true;
      }
    }

    // If we found a duplicate, remove it.
    if (duplicate) {
      rhs = nullptr;
    } else {
      // Mark rhs as seen.
      seen.push_back(rhs);
    }
  }

  // If nothing changed, return the original.
  if (lhs == &C.lhs() and rhs == &C.rhs()) {
    return C;
  }

  // Otherwise, construct the new content.
  if (lhs == nullptr) {
    if (rhs == nullptr) {
      return Content::create<EmptyContent>();
    } else {
      return *rhs;
    }
  } else {
    if (rhs == nullptr) {
      return *lhs;
    }
  }
  return Content::create<UnionContent>(*lhs, *rhs);
}
} // namespace detail

Content &ContentSimplification::visitUnionContent(UnionContent &C) {

  // Deduplicate.
  vector<Content *> seen = {};
  auto &union_content = detail::dedup(C, seen);
  if (&union_content != &C) {
    return union_content;
  }

  // Unpack the union.
  auto &lhs = this->visit(C.lhs());
  auto &rhs = this->visit(C.rhs());

  // C U empty = C
  if (isa<EmptyContent>(&rhs)) {
    return lhs;
  }

  // empty U C = C
  if (isa<EmptyContent>(&lhs)) {
    return rhs;
  }

  // C U C = C
  if (lhs == rhs) {
    return lhs;
  }

  // [ ..., x, ... ] U [ ..., empty, ... ] = [ ..., x, ... ]
  auto *lhs_tuple = dyn_cast<TupleContent>(&lhs);
  auto *rhs_tuple = dyn_cast<TupleContent>(&rhs);
  if (lhs_tuple and rhs_tuple) {
    // For each element, merge empty contents.
    auto &lhs_elements = lhs_tuple->elements();
    auto &rhs_elements = rhs_tuple->elements();

    MEMOIR_ASSERT(lhs_elements.size() == rhs_elements.size(),
                  "TupleContents being union'd are not same size.");

    auto size = lhs_elements.size();

    llvm::memoir::vector<Content *> new_elements = {};
    new_elements.reserve(size);
    for (size_t i = 0; i < size; ++i) {
      auto &lhs_elem = this->visit(lhs_tuple->element(i));
      auto &rhs_elem = this->visit(rhs_tuple->element(i));

      // TODO: visit lhs_elem and rhs_elem

      if (isa<EmptyContent>(&lhs_elem)) {
        new_elements.push_back(&rhs_elem);
      } else if (isa<EmptyContent>(&rhs_elem)) {
        new_elements.push_back(&lhs_elem);
      } else if (lhs_elem == rhs_elem) {
        new_elements.push_back(&lhs_elem);
      } else {
        // If there is a collision, we cannot visit to a single tuple.
        return C;
      }
    }

    // Construct a new tuple.
    auto &new_tuple = Content::create<TupleContent>(new_elements);

    return new_tuple;
  }

  // (x | c1) U (empty | c2) = (x | c1)
  auto *lhs_cond = dyn_cast<ConditionalContent>(&lhs);
  auto *rhs_cond = dyn_cast<ConditionalContent>(&rhs);
  if (lhs_cond and rhs_cond) {
    auto &lhs_content = lhs_cond->content();
    auto &rhs_content = rhs_cond->content();

    if (isa<EmptyContent>(&lhs_content)) {
      return *rhs_cond;
    }

    if (isa<EmptyContent>(&rhs_content)) {
      return *lhs_cond;
    }
  }

  // If nothing happened, then see if we can reassociate the union and go
  // again.
  if (auto *rhs_union = dyn_cast<UnionContent>(&lhs)) {
    auto &rhs_lhs = rhs_union->lhs();
    auto &rhs_rhs = rhs_union->rhs();
    auto &new_union = this->visit(Content::create<UnionContent>(lhs, rhs_lhs));

    return this->visit(Content::create<UnionContent>(new_union, rhs_rhs));
  }

  // If we weren't able to reassociate, create a new union content if any
  // sub-simplifications happened.
  if (&lhs != &C.lhs() or &rhs != &C.rhs()) {
    return this->visit(Content::create<UnionContent>(lhs, rhs));
  }

  return C;
}

} // namespace folio
