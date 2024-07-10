#ifndef MEMOIR_IR_PROPERTY_H
#define MEMOIR_IR_PROPERTY_H

#include "memoir/ir/Instructions.hpp"

namespace llvm::memoir {

enum PropertyKind {
#define HANDLE_PROPERTY(ID, ENUM, CLASS) ENUM,
  PROPERTY_NONE,
};

struct Property {
public:
  // Constructor.
  Property(const PropertyInst &inst) : inst(&inst) {}

  // Accessors.
  const PropertyInst &getPropertyInst() const {
    return *this->inst;
  }

  // Copy constructor.
  Property(Property &) = default;

  // Move constructor.
  Property(Property &&) = default;

  // Copy assignment constructor.
  Property &operator=(Property &) = default;

  // Move constructor.
  Property &operator=(Property &&) = default;

  // Destructor.
  ~Property() = default;

protected:
  const PropertyInst *inst;
};

} // namespace llvm::memoir

#endif // MEMOIR_IR_PROPERTY_H
