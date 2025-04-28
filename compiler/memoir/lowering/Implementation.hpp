#ifndef MEMOIR_LOWERING_IMPLEMENATION_H
#define MEMOIR_LOWERING_IMPLEMENATION_H

#include <string>

#include "memoir/ir/Types.hpp"

#include "memoir/support/AssocList.hpp"

namespace llvm::memoir {

// Forward definitions.
struct Instantiation;

/**
 * An Implementation holds a unique name and a type template specifying which
 * object types it can match on.
 */
struct Implementation {
public:
  static void define(Implementation impl) {
    if (Implementation::templates == nullptr) {
      Implementation::templates = new Map<std::string, Implementation>;
    }
    Implementation::templates->insert({ impl.get_name(), impl });
  }

  static void define(std::initializer_list<Implementation> impl_list) {
    if (Implementation::templates == nullptr) {
      Implementation::templates = new Map<std::string, Implementation>;
    }
    for (const auto &impl : impl_list) {
      Implementation::templates->insert({ impl.get_name(), impl });
    }
  }

  static Implementation *lookup(std::string name) {
    if (Implementation::templates == nullptr) {
      return nullptr;
    }

    auto &templates = *Implementation::templates;
    auto found = templates.find(name);
    if (found == templates.end()) {
      return nullptr;
    }

    return &found->second;
  }

  static void candidates(Type &type,
                         Vector<const Implementation *> &candidates) {
    if (not Implementation::templates) {
      return;
    }

    for (const auto &[_name, impl] : *Implementation::templates) {
      if (auto *collection_type = dyn_cast<CollectionType>(&type)) {
        auto existing = collection_type->get_selection();
        if (existing) {
          if (existing.value() == impl.get_name()) {
            candidates.push_back(&impl);
            continue;
          } else if ((existing.value() == "bitset"
                      or existing.value() == "bitmap")
                     and impl.get_name().find(existing.value())
                             != std::string::npos) {
            candidates.push_back(&impl);
            continue;
          } else {
            continue;
          }
        }
      }

      if (impl.selectable() and impl.match(type)) {
        candidates.push_back(&impl);
      }
    }
  }

  static Vector<const Implementation *> candidates(Type &type) {
    Vector<const Implementation *> candidates = {};

    Implementation::candidates(type, candidates);

    return candidates;
  }

  /**
   * Checks if the given type can be matched to this implementation's
   * template.
   *
   * @param to_match the type to be matched against
   * @returns true if the type matches the template, false otherwise
   */
  bool match(Type &to_match,
             AssocList<TypeVariable *, Type *> *env = nullptr) const;

  /**
   * @returns the number of dimensions that this implementation covers.
   */
  unsigned num_dimensions() const;

  /**
   * @returns a new implementation, instantiated for the provided type.
   */
  Instantiation &instantiate(Type &type) const;

  Implementation(std::string name, Type &type_template, bool selectable = true)
    : _name(name),
      _template(&type_template),
      _selectable(selectable) {}

  std::string get_name() const {
    return this->_name;
  }

  Type &get_template() const {
    return *this->_template;
  }

  bool selectable() const {
    return this->_selectable;
  }

protected:
  std::string _name;
  Type *_template;
  bool _selectable;

  static Map<std::string, Implementation> *templates;
};

struct Instantiation : public Implementation {

  /**
   * Fetches or constructs the given instantiation.
   */
  static Instantiation &instantiate(
      std::string name,
      Type &type_template,
      const AssocList<TypeVariable *, Type *> &types);

  /**
   * @returns the function/type prefix for this instantiation.
   */
  std::string get_prefix() const;

  /**
   * @returns the C type for this instantiation.
   */
  std::string get_typename() const;

  Vector<Type *> &types() {
    return this->_types;
  }

  const Vector<Type *> &types() const {
    return this->_types;
  }

protected:
  Vector<Type *> _types;

  Instantiation(std::string name, Type &type_template)
    : Implementation(name, type_template),
      _types() {}

  static OrderedMultiMap<std::string, Instantiation *> *instantiations;
};

} // namespace llvm::memoir

#endif
