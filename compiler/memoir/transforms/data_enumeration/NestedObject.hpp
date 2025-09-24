#ifndef FOLIO_TRANSFORMS_NESTEDOBJECT_H
#define FOLIO_TRANSFORMS_NESTEDOBJECT_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/Value.h"

#include "memoir/support/DataTypes.hpp"

namespace memoir {

using Offset = unsigned;
using Offsets = typename SmallVector<Offset>;

struct NestedObject {
  // Accessors.
  llvm::Value &value() const {
    return *this->_value;
  }

  llvm::ArrayRef<Offset> offsets() const {
    return this->_offsets;
  }

  // Constructors.
  NestedObject(llvm::Value &value, llvm::ArrayRef<unsigned> offsets = {})
    : _value(&value),
      _offsets(offsets) {}

  friend bool operator<(const NestedObject &lhs, const NestedObject &rhs);
  friend bool operator==(const NestedObject &lhs, const NestedObject &rhs);

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                       const NestedObject &obj);

protected:
  llvm::Value *_value;
  Offsets _offsets;
};

} // namespace memoir

template <>
struct std::hash<folio::NestedObject> {
  std::size_t operator()(const folio::NestedObject &obj) const noexcept {
    std::size_t h1 = std::hash<llvm::Value *>{}(&obj.value());
    std::size_t i = 0;
    for (auto offset : obj.offsets()) {
      std::size_t h2 = std::hash<unsigned>{}(offset);
      h1 ^= (h2 << ++i);
    }
    return h1;
  }
};

#endif
