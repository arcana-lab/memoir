#include "memoir/lowering/Implementation.hpp"

namespace llvm::memoir {

namespace detail {

bool match(map<TypeVariable *, Type *> &environment,
           Type &to_match,
           Type &pattern) {

  // If these are the same type objects, they match.
  if (&to_match == &pattern) {
    return true;
  }

  // Check if the type variable in the pattern matches, checking with
  // constraints in the environment.
  if (auto *var = dyn_cast<TypeVariable>(&pattern)) {
    auto found = environment.find(var);
    if (found == environment.end()) {
      environment[var] = &to_match;
    } else {
      auto &binding = *found->second;
      return match(environment, to_match, binding);
    }

    return true;
  }

  // Check the type codes.
  if (to_match.getKind() != pattern.getKind()) {
    return false;
  }

  // Check type constructors.
  if (auto *seq = dyn_cast<SequenceType>(&pattern)) {
    auto *seq_to_match = cast<SequenceType>(&to_match);

    return match(environment,
                 seq_to_match->getElementType(),
                 seq->getElementType());
  } else if (auto *assoc = dyn_cast<AssocType>(&pattern)) {
    auto *assoc_to_match = cast<AssocType>(&to_match);

    return match(environment, assoc_to_match->getKeyType(), assoc->getKeyType())
           and match(environment,
                     assoc_to_match->getElementType(),
                     assoc->getElementType());
  }

  // If we got this far, it conservatively doesn't match.
  return false;
}

} // namespace detail

bool Implementation::match(Type &to_match) const {
  map<TypeVariable *, Type *> environment = {};
  return detail::match(environment, to_match, this->get_template());
}

unsigned Implementation::num_dimensions() const {
  auto *type = &this->get_template();

  unsigned n;
  for (n = 0; isa<CollectionType>(type); ++n) {
    auto *collection_type = cast<CollectionType>(type);
    type = &collection_type->getElementType();
  }
  return n;
}

map<std::string, Implementation> *Implementation::impls = nullptr;

} // namespace llvm::memoir
