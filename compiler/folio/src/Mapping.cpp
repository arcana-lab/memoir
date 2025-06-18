#include "folio/Mapping.hpp"

using namespace llvm::memoir;

namespace folio {

// Allocation accessors.
llvm::Value &Mapping::alloc() const {
  return *this->_alloc;
}

void Mapping::alloc(llvm::Value &V) {
  this->_alloc = &V;
}

// Globals accessors.
llvm::GlobalVariable &Mapping::global(llvm::Value *base) const {
  return *this->_globals.at(base);
}

void Mapping::global(llvm::Value *base, llvm::GlobalVariable &GV) {
  this->_globals[base] = &GV;
}

Mapping::GlobalsMap &Mapping::globals() {
  return this->_globals;
}

const Mapping::GlobalsMap &Mapping::globals() const {
  return this->_globals;
}

// Locals accessors.
llvm::AllocaInst &Mapping::local(llvm::Value *base) const {
  return *this->_locals.at(base);
}

void Mapping::local(llvm::Value *base, llvm::AllocaInst &stack) {
  this->_locals[base] = &stack;
}

Mapping::LocalsMap &Mapping::locals() {
  return this->_locals;
}

const Mapping::LocalsMap &Mapping::locals() const {
  return this->_locals;
}

} // namespace folio
