#ifndef FOLIO_SOLVER_SELECTION_H
#define FOLIO_SOLVER_SELECTION_H

#include "memoir/ir/Types.hpp"

#include "folio/solver/Implementation.hpp"

namespace folio {

/**
 * A Selection represents the implementation and type of a collection for
 * lowering.
 */
struct Selection {
public:
  Selection(Implementation &impl, llvm::memoir::CollectionType &type)
    : _impl(impl),
      _type(type) {}

  Implementation &implementation() {
    return this->_impl;
  }

  llvm::memoir::CollectionType &type() {
    return this->_type;
  }

protected:
  Implementation &_impl;
  llvm::memoir::CollectionType &_type;
};

} // namespace folio

#endif // FOLIO_SOLVER_SELECTION_H
