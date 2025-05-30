#ifndef MEMOIR_ANALYSIS_BOUNDSCHECKANALYSIS_H
#define MEMOIR_ANALYSIS_BOUNDSCHECKANALYSIS_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/Value.h"

#include "memoir/passes/Passes.hpp"
#include "memoir/support/DataTypes.hpp"

namespace llvm::memoir {

struct BoundsCheck {
public:
  // Accessors
  llvm::Value &key() const;

  llvm::ArrayRef<llvm::Value *> offset() const;

  // Comparison.
  friend bool operator<(const BoundsCheck &lhs, const BoundsCheck &rhs);

  // Constructors.
  BoundsCheck(llvm::Value &key) : BoundsCheck(key, {}) {}
  BoundsCheck(llvm::ArrayRef<llvm::Value *> indices)
    : BoundsCheck(*indices.back(), indices.drop_back()) {}
  BoundsCheck(llvm::Value &key, llvm::ArrayRef<llvm::Value *> offset)
    : _key(&key),
      _offset(offset.begin(), offset.end()) {}

protected:
  llvm::Value *_key;
  Vector<llvm::Value *> _offset;
};

struct BoundsChecks : public OrderedSet<BoundsCheck> {

  void merge() {}
};

struct BoundsCheckResult {
public:
  BoundsCheckResult() : _facts{ { NULL, {} } } {}

  const BoundsChecks &facts(llvm::Value &value);

protected:
  Map<llvm::Value *, BoundsChecks> _facts;
};

} // namespace llvm::memoir

#endif
