#include "memoir/lowering/Implementation.hpp"

namespace llvm::memoir {

namespace detail {

bool match(AssocList<TypeVariable *, Type *> &environment,
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

bool Implementation::match(Type &to_match,
                           AssocList<TypeVariable *, Type *> *env) const {

  if (env) {
    return detail::match(*env, to_match, this->get_template());
  }

  AssocList<TypeVariable *, Type *> local_env = {};
  return detail::match(local_env, to_match, this->get_template());
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

std::string Implementation::get_prefix(Type &type) const {
  // Constructs the function/type prefix of the implementation by matching the
  // type variables in the template.
  AssocList<TypeVariable *, Type *> environment = {};
  MEMOIR_ASSERT(this->match(type, &environment),
                "Failed to match implementation template");

  // Construct the prefix by listing the bound type variables in order.
  std::string prefix = "";
  for (const auto &binding : environment) {
    if (auto binding_code = binding.second->get_code()) {
      prefix += binding_code.value() + "_";
    } else {
      MEMOIR_UNREACHABLE("Bound type in template has no code!");
    }
  }

  // Finally, append the name of the implementation
  prefix += this->get_name();

  return prefix;
}

map<std::string, Implementation> *Implementation::impls = nullptr;

} // namespace llvm::memoir
