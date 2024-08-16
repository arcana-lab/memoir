#ifndef FOLIO_IMPLEMENTATION_H
#define FOLIO_IMPLEMENTATION_H

#include <string>

#include "memoir/support/InternalDatatypes.hpp"

#include "folio/analysis/Constraints.hpp"

namespace folio {

enum ImplementationKind {
  IMPLEMENTATION_SEQ,
  IMPLEMENTATION_ASSOC,
};

struct Implementation {
public:
  Implementation(ImplementationKind kind,
                 std::string name,
                 std::initializer_list<Constraint> &&constraints)
    : _kind{ kind },
      _name{ name },
      _constraints{ std::forward<std::initializer_list<Constraint>>(
          constraints) } {}

  ImplementationKind kind() const {
    return this->_kind;
  }

  std::string name() const {
    return this->_name;
  }

  /**
   * Query the set of constraints that are _not_ met by this implementation.
   */
  const llvm::memoir::ordered_set<Constraint> &constraints() const {
    return this->_constraints;
  }

  bool operator==(Implementation other) const {
    return (this->_kind == other.kind()) && (this->_name == other.name());
  }

  bool operator<(Implementation other) const {
    return (this->_name < other.name());
  }

protected:
  ImplementationKind _kind;
  std::string _name;
  llvm::memoir::ordered_set<Constraint> _constraints;
};

struct SeqImplementation : public Implementation {
public:
  SeqImplementation(std::string name,
                    std::initializer_list<Constraint> &&constraints)
    : Implementation(
          ImplementationKind::IMPLEMENTATION_SEQ,
          name,
          std::forward<std::initializer_list<Constraint>>(constraints)) {}

  static bool classof(const Implementation *other) {
    return other->kind() == ImplementationKind::IMPLEMENTATION_SEQ;
  }
};

struct AssocImplementation : public Implementation {
public:
  AssocImplementation(std::string name,
                      std::initializer_list<Constraint> &&constraints)
    : Implementation(
          ImplementationKind::IMPLEMENTATION_ASSOC,
          name,
          std::forward<std::initializer_list<Constraint>>(constraints)) {}

  static bool classof(const Implementation *other) {
    return other->kind() == ImplementationKind::IMPLEMENTATION_ASSOC;
  }
};

using Implementations = typename llvm::memoir::map<std::string, Implementation>;

} // namespace folio

#endif
