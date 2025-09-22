#include "memoir/analysis/BoundsCheckAnalysis.hpp"

namespace memoir {

// Accessors
llvm::Value &BoundsCheck::key() const {
  return MEMOIR_SANITIZE(this->_indices.back(), "Bounds check has no indices!");
}

llvm::ArrayRef<llvm::Value *> BoundsCheck::indices() const {
  return this->_indices;
}

bool BoundsCheck::negated() const {
  return this->_negate;
}

void BoundsCheck::flip() {
  this->_negate = not this->_negate;
}

// Comparison.
static std::optional<bool> index_order(const BoundsCheck &lhs,
                                       const BoundsCheck &rhs) {

  // Order by offset length first.
  if (lhs.indices().size() < rhs.indices().size()) {
    return true;
  }

  // Order by indicess values next.
  auto lit = lhs.indices().begin(), lie = lhs.indices().end();
  auto rit = rhs.indices().begin(), rie = rhs.indices().end();
  for (; lit != lie; ++lit, ++rit) {
    if (*lit < *rit) {
      return true;
    } else if (*rit < *lit) {
      return false;
    }
  }

  return {};
}

bool operator<(const BoundsCheck &lhs, const BoundsCheck &rhs) {
  // Order by indices.
  if (auto order = index_order(lhs, rhs)) {
    return *order;
  }

  // Order by negation.
  return lhs.negated() < rhs.negated();
}

bool BoundsCheck::TotalOrder::operator()(const BoundsCheck &lhs,
                                         const BoundsCheck &rhs) {
  return lhs < rhs;
}

bool BoundsCheck::ConflictOrder::operator()(const BoundsCheck &lhs,
                                            const BoundsCheck &rhs) {
  // Order by indices.
  if (auto order = index_order(lhs, rhs)) {
    return *order;
  }

  // Modulo negation.
  return false;
}

// BoundsCheckSet
BoundsCheckSet::Diff BoundsCheckSet::join(const BoundsCheckSet &other) {

  // Track the number of values inserted.
  auto orig_size = this->size();

  // Merge the other vector into this vector.
  auto it = this->begin();
  for (const auto &v : other) {
    // Get the range of equal values in the vector.
    auto [lower, upper] =
        std::equal_range(it, this->end(), v, BoundsCheck::ConflictOrder{});

    // If the lower and upper bounds are the same, then we have a unique value
    // to insert.
    if (lower == upper) {

      // Insert the value at the upper bound.
      auto ins_it = this->Base::insert(upper, v);

      // Continue insertion after the inserted element.
      it = std::next(ins_it);
      continue;
    }

    // If the lower and upper bounds are different, then we have a possible
    // conflict. Check that the items don't negate one another.
    if (lower->negated() != v.negated()) {
      // If they negate one another, remove the existing item.
      it = this->Base::erase(lower);
      continue;
    }

    // Otherwise, continue.
    it = upper;
    continue;
  }

  // Return the number of values inserted.
  return this->size() - orig_size;
}

BoundsCheckSet::Diff BoundsCheckSet::meet(const BoundsCheckSet &other) {

  // TODO: convert this to an in-place update by iterating over this and
  // checking the equal_range in other.

  // Construct a temporary set.
  BoundsCheckSet intersection;

  // Compute the set intersection.
  std::set_intersection(this->cbegin(),
                        this->cend(),
                        other.cbegin(),
                        other.cend(),
                        std::back_inserter(intersection));

  // Check the number of elements removed.
  auto diff = intersection.size() - this->size();

  // If the size is the same as the original, then we haven't changed.
  if (diff == 0) {
    return diff;
  }

  // Otherwise, we have changed. Copy the temporary set to this.
  *this = intersection;

  // Return the magnitude of the difference.
  return diff;
}

BoundsCheckSet::Diff BoundsCheckSet::insert(const BoundsCheck &check) {
  auto orig_size = this->size();

  auto [lower, upper] = std::equal_range(this->begin(), this->end(), check);

  if (lower == upper) {
    // If the check is not in the set yet, insert it.
    this->Base::insert(upper, check);

  } else if (lower != upper) {
    // If the negation of the check is in the set, replace it.
    if (lower->negated() != check.negated()) {
      lower->flip();
    }
  }

  return this->size() - orig_size;
}

// BoundsCheckResult
void BoundsCheckResult::initialize(llvm::Value &value,
                                   const BoundsCheck &init) {
  this->Base::emplace(&value, BoundsCheckSet(init));
}

void BoundsCheckResult::initialize(llvm::Value &value,
                                   llvm::ArrayRef<BoundsCheck> init) {
  this->Base::emplace(&value, BoundsCheckSet(init));
}

BoundsCheckSet &BoundsCheckResult::operator[](llvm::Value &value) {
  return this->Base::operator[](&value);
}

const BoundsCheckSet &BoundsCheckResult::operator[](llvm::Value &value) const {
  return this->Base::at(&value);
}

bool BoundsCheckResult::contains(llvm::Value &value) const {
  return this->Base::count(&value) > 0;
}

// Printing.

llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const BoundsCheck &check) {

  if (check.negated()) {
    os << "¬";
  }

  os << "[";

  bool first = true;
  for (auto *val : check.indices()) {
    if (first) {
      first = false;
    } else {
      os << ", ";
    }
    val->printAsOperand(os, /* print type? */ false);
  }

  os << "]";

  return os;
}

llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                              const BoundsCheckSet &set) {
  os << "{ ";
  bool first = true;
  for (const auto &check : set) {
    if (first) {
      first = false;
    } else {
      os << ", ";
    }
    os << check;
  }
  os << " }";

  return os;
}

llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                              const BoundsCheckResult &result) {

  for (const auto &[value, set] : result) {
    value->printAsOperand(os, /* print type? */ false);
    os << " ⊢ " << set << "\n";
  }

  return os;
}

} // namespace memoir
