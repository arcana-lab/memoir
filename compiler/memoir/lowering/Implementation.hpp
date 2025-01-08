#ifndef MEMOIR_LOWERING_IMPLEMENATION_H
#define MEMOIR_LOWERING_IMPLEMENATION_H

#include <string>

#include "memoir/ir/Types.hpp"

namespace llvm::memoir {

/**
 * An Implementation holds a unique name and a type template specifying which
 * object types it can match on.
 */
struct Implementation {
public:
  static void define(Implementation impl) {
    if (Implementation::impls == nullptr) {
      Implementation::impls = new map<std::string, Implementation>;
    }
    Implementation::impls->insert({ impl.get_name(), impl });
  }

  static void define(std::initializer_list<Implementation> impl_list) {
    if (Implementation::impls == nullptr) {
      Implementation::impls = new map<std::string, Implementation>;
    }
    for (const auto &impl : impl_list) {
      Implementation::impls->insert({ impl.get_name(), impl });
    }
  }

  static Implementation *lookup(std::string name) {
    if (Implementation::impls == nullptr) {
      return nullptr;
    }

    auto &impls = *Implementation::impls;
    auto found = impls.find(name);
    if (found == impls.end()) {
      return nullptr;
    }

    return &found->second;
  }

  /**
   * Checks if the given type can be matched to this implementation's
   * template.
   *
   * @param to_match the type to be matched against
   * @returns true if the type matches the template, false otherwise
   */
  bool match(Type &to_match) const;

  /**
   * @returns the number of dimensions that this implementation covers.
   */
  unsigned num_dimensions() const;

  std::string get_name() const {
    return this->name;
  }

  Type &get_template() const {
    return *this->type_template;
  }

protected:
  Implementation(std::string name, Type &type_template)
    : name(name),
      type_template(&type_template) {}

  std::string name;
  Type *type_template;

  static map<std::string, Implementation> impls;
};

} // namespace llvm::memoir

#endif
