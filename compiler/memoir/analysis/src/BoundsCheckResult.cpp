#include "memoir/analysis/BoundsCheckAnalysis.hpp"

namespace llvm::memoir {

// Accessors
llvm::Value &BoundsCheck::key() const {
  return *this->_key;
}

llvm::ArrayRef<llvm::Value *> BoundsCheck::offset() const {
  return this->_offset;
}

// Comparison.
bool operator<(const BoundsCheck &lhs, const BoundsCheck &rhs) {
  // Order by offset length first.
  if (lhs.offset().size() < rhs.offset().size()) {
    return true;
  }

  // Order by offsets values next.
  auto lit = lhs.offset().begin(), lie = lhs.offset().end();
  auto rit = rhs.offset().begin(), rie = rhs.offset().end();
  for (; lit != lie; ++lit, ++rit) {
    if (*lit < *rit) {
      return true;
    } else if (*rit < *lit) {
      return false;
    }
  }

  // Order by keys.
  return &lhs.key() < &rhs.key();
}
} // namespace llvm::memoir
