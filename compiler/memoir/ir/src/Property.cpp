#include "memoir/ir/Property.hpp"

namespace llvm::memoir {

// Property base class implementation.
const PropertyInst &Property::getPropertyInst() const {
  return *this->inst;
}

} // namespace llvm::memoir
