#ifndef FOLIO_IMPLEMENTATION_H
#define FOLIO_IMPLEMENTATION_H

#include <string>

#include "memoir/support/DataTypes.hpp"

#include "folio/analysis/Constraints.hpp"

namespace folio {

enum ImplementationKind {
  IMPLEMENTATION_SEQ,
  IMPLEMENTATION_ASSOC,
  IMPLEMENTATION_SET,
};

struct Implementation {
public:
  Implementation(ImplementationKind kind,
                 std::string name,
                 std::initializer_list<Constraint> &&constraints,
                 bool selectable = true)
    : _kind{ kind },
      _name{ name },
      _constraints{ std::forward<std::initializer_list<Constraint>>(
          constraints) },
      _selectable(selectable) {}

  ImplementationKind kind() const {
    return this->_kind;
  }

  std::string name() const {
    return this->_name;
  }

  /**
   * Query the set of constraints that are _not_ met by this implementation.
   */
  const llvm::memoir::OrderedSet<Constraint> &constraints() const {
    return this->_constraints;
  }

  bool selectable() const {
    return this->_selectable;
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
  llvm::memoir::OrderedSet<Constraint> _constraints;
  bool _selectable;
};

struct SeqImplementation : public Implementation {
public:
  SeqImplementation(std::string name,
                    std::initializer_list<Constraint> &&constraints,
                    bool selectable = true)
    : Implementation(
          ImplementationKind::IMPLEMENTATION_SEQ,
          name,
          std::forward<std::initializer_list<Constraint>>(constraints),
          selectable) {}

  static bool classof(const Implementation *other) {
    return other->kind() == ImplementationKind::IMPLEMENTATION_SEQ;
  }
};

struct AssocImplementation : public Implementation {
public:
  AssocImplementation(std::string name,
                      std::initializer_list<Constraint> &&constraints,
                      bool selectable = true)
    : Implementation(
          ImplementationKind::IMPLEMENTATION_ASSOC,
          name,
          std::forward<std::initializer_list<Constraint>>(constraints),
          selectable) {}

  static bool classof(const Implementation *other) {
    return other->kind() == ImplementationKind::IMPLEMENTATION_ASSOC;
  }
};

struct SetImplementation : public Implementation {
public:
  SetImplementation(std::string name,
                    std::initializer_list<Constraint> &&constraints,
                    bool selectable = true)
    : Implementation(
          ImplementationKind::IMPLEMENTATION_SET,
          name,
          std::forward<std::initializer_list<Constraint>>(constraints),
          selectable) {}

  static bool classof(const Implementation *other) {
    return other->kind() == ImplementationKind::IMPLEMENTATION_SET;
  }
};

using Implementations = typename llvm::memoir::Map<std::string, Implementation>;

} // namespace folio

#endif
