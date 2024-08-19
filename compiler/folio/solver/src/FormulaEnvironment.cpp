#include "folio/solver/Solver.hpp"

using namespace llvm::memoir;

namespace folio {

llvm::Value &FormulaEnvironment::lookup(uint32_t id) const {
  auto found = this->_id_values.find(id);
  if (found == this->_id_values.end()) {
    MEMOIR_UNREACHABLE("No value found for given identifier.");
  }

  return *(found->second);
}

uint32_t FormulaEnvironment::get_id(llvm::Value &V) {
  // Search for an existing identifer.
  auto found = this->_value_ids.find(&V);
  if (found != this->_value_ids.end()) {
    return found->second;
  }

  // If we failed to find, create a new ID.
  auto id = this->_current_id++;
  this->_value_ids[&V] = id;
  this->_id_values[id] = &V;

  return id;
}

} // namespace folio
