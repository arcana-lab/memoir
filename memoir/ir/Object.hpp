#ifndef MEMOIR_IR_OBJECT_H
#define MEMOIR_IR_OBJECT_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/Value.h"

#include "memoir/ir/Types.hpp"
#include "memoir/support/DataTypes.hpp"

namespace memoir {

using Offset = unsigned;
using Offsets = SmallVector<Offset>;
using OffsetsRef = typename llvm::ArrayRef<Offset>;

struct Object {
  /** The SSA value of this object. */
  llvm::Value &value() const;

  /** The offset of the object within the SSA value. */
  llvm::ArrayRef<Offset> offsets() const;

  /** Utility to get the type of an object nested within an SSA value. */
  static Type &type(llvm::Value &value, OffsetsRef offsets);

  /** Utility to get the type of a nested object from a type. */
  static Type &type(Type &type, OffsetsRef offsets);

  /** Type of the object */
  Type &type() const;

  /** Parent basic block of the object, if it exists */
  llvm::BasicBlock *block() const;

  /** Parent function of the object, if it exists */
  llvm::Function *function() const;

  /** Parent module of the object. */
  llvm::Module &module() const;

  // Constructors.
  Object(llvm::Value &value, OffsetsRef offsets = {})
    : _value(&value),
      _offsets(offsets) {}

  friend bool operator<(const Object &lhs, const Object &rhs);
  friend bool operator==(const Object &lhs, const Object &rhs);

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                       const Object &obj);

protected:
  llvm::Value *_value;
  Offsets _offsets;

  void value(llvm::Value &new_value);
};

} // namespace memoir

template <>
struct std::hash<memoir::Object> {
  std::size_t operator()(const memoir::Object &obj) const noexcept;
};

#endif // MEMOIR_IR_OBJECT_H
