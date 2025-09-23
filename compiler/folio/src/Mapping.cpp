#include "folio/Mapping.hpp"

using namespace memoir;

namespace folio {

// Globals accessors.
llvm::GlobalVariable &Mapping::global(ObjectInfo &base) const {
  return *this->_globals.at(&base);
}

void Mapping::global(ObjectInfo &base, llvm::GlobalVariable &GV) {
  this->_globals[&base] = &GV;
}

Mapping::GlobalsMap &Mapping::globals() {
  return this->_globals;
}

const Mapping::GlobalsMap &Mapping::globals() const {
  return this->_globals;
}

// Locals accessors.
llvm::AllocaInst &Mapping::local(ObjectInfo &base) const {
  return *this->_locals.at(&base);
}

void Mapping::local(ObjectInfo &base, llvm::AllocaInst &stack) {
  this->_locals[&base] = &stack;
}

Mapping::LocalsMap &Mapping::locals() {
  return this->_locals;
}

const Mapping::LocalsMap &Mapping::locals() const {
  return this->_locals;
}

} // namespace folio
