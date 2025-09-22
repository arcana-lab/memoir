#ifndef MEMOIR_ANALYSIS_BOUNDSCHECKANALYSIS_H
#define MEMOIR_ANALYSIS_BOUNDSCHECKANALYSIS_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/Value.h"

#include "memoir/ir/Instructions.hpp"
#include "memoir/passes/Passes.hpp"
#include "memoir/support/DataTypes.hpp"

namespace memoir {

struct BoundsCheck {
public:
  // Accessors
  llvm::Value &key() const;
  llvm::ArrayRef<llvm::Value *> indices() const;
  bool negated() const;

  // Updates.
  void flip();

  // Constructors.
  BoundsCheck(llvm::Value &key, bool negate = false)
    : BoundsCheck({ &key }, negate) {}
  BoundsCheck(llvm::ArrayRef<llvm::Value *> indices, bool negate = false)
    : _indices(indices.begin(), indices.end()),
      _negate(negate) {}
  BoundsCheck(AccessInst &access, bool negate = false)
    : _indices(access.indices_begin(), access.indices_end()),
      _negate(negate) {}

  // Comparison.
  friend bool operator<(const BoundsCheck &lhs, const BoundsCheck &rhs);

  /**
   * Basic comparison object that provides a total order.
   */
  struct TotalOrder {
    bool operator()(const BoundsCheck &lhs, const BoundsCheck &rhs);
  };

  /**
   * A special comparison object that provides a total order modulo negation.
   */
  struct ConflictOrder {
    bool operator()(const BoundsCheck &lhs, const BoundsCheck &rhs);
  };

  // Printing.
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &,
                                       const BoundsCheck &);

protected:
  Vector<llvm::Value *> _indices;
  bool _negate;
};

// We will implement our bounds checks as a sorted vector for fast insertion.
struct BoundsCheckSet : Vector<BoundsCheck> {
protected:
  using Base = Vector<BoundsCheck>;
  using Size = Base::size_type;
  using Diff = Base::difference_type;

public:
  BoundsCheckSet() : Base{} {}
  BoundsCheckSet(const BoundsCheck &check) : Base{ check } {}
  BoundsCheckSet(llvm::ArrayRef<BoundsCheck> init)
    : Base(init.begin(), init.end()) {}

  //   a join b
  //     /  \
  //    a    b
  //     \  /
  //   a meet b

  /**
   * Compute the join (maximum) of two sets in-place.
   * @returns the number of elements changed
   */
  Diff join(const BoundsCheckSet &other);

  /**
   * Compute the meet (minimum) of two sets, in-place.
   * @returns the number of differences
   */
  Diff meet(const BoundsCheckSet &other);

  /**
   * Insert a fact into the set.
   * @returns the number of differences
   */
  Diff insert(const BoundsCheck &check);

  // Accessors.
  using Base::empty;
  using Base::size;

  // Iterators.
  using Base::begin;
  using Base::cbegin;
  using Base::cend;
  using Base::end;

  // Printing.
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &,
                                       const BoundsCheckSet &);
};

struct BoundsCheckResult : Map<llvm::Value *, BoundsCheckSet> {
  using Base = Map<llvm::Value *, BoundsCheckSet>;

public:
  BoundsCheckResult() : Base{} {}

  bool contains(llvm::Value &value) const;

  BoundsCheckSet &operator[](llvm::Value &value);
  const BoundsCheckSet &operator[](llvm::Value &value) const;

  void initialize(llvm::Value &value, const BoundsCheck &init);
  void initialize(llvm::Value &value, llvm::ArrayRef<BoundsCheck> init = {});

  // Accessors.
  using Base::empty;
  using Base::size;

  // Iterators.
  using Base::begin;
  using Base::cbegin;
  using Base::cend;
  using Base::end;

  // Printing.
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &,
                                       const BoundsCheckResult &);
};

} // namespace memoir

#endif
