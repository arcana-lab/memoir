#ifndef MEMOIR_SUPPORT_SORTEDVECTOR_H
#define MEMOIR_SUPPORT_SORTEDVECTOR_H

#include "memoir/support/SortedVector.hpp"

namespace memoir {

template <typename T, typename Cmp = std::less<T>>
struct SortedVector : public Vector<T> {
  using Self = SortedVector<T>;
  using Base = Vector<T>;

  // Hide Base::insert with a sorted insert.
  bool insert(const T &val) {
    auto [lower, upper] =
        std::equal_range(this->begin(), this->end(), val, Cmp{});
    if (lower == upper) {
      this->Base::insert(lower, val);
      return true;
    }
    return false;
  }

  template <typename RangeCmp = Cmp>
  auto equal_range(const T &val) {
    return std::equal_range(this->begin(), this->end(), val, RangeCmp{});
  }

  template <typename FindCmp = Cmp>
  auto find(const T &val) {
    auto [lower, upper] = this->equal_range<FindCmp>(val);

    // If the bounds are equal, we failed to find.
    if (lower == upper) {
      return this->end();
    }

    // Otherwise, return the lower bound.
    return lower;
  }

  bool set_union(const Self &other) {
    bool modified = false;

    auto it = this->begin();
    for (const auto &val : other) {
      auto [lower, upper] = std::equal_range(it, this->end(), val, Cmp{});

      if (lower == upper) {
        // If the value is not in the range, insert it.
        it = std::next(this->Base::insert(upper, val));
        modified |= true;
      } else {
        // Otherwise, continue iterating.
        it = upper;
      }
    }

    return modified;
  }

  bool set_intersection(const Self &other) {

    bool modified = false;

    // Remove all elements that do not occur in other.
    auto oit = other.cbegin(), oie = other.cend();

    for (auto it = this->begin(); it != this->end();) {

      auto [lower, upper] = std::equal_range(oit, oie, *it, Cmp{});

      if (lower == upper) {
        // If the value is not in the range, remove it.
        it = this->Base::erase(it);
        modified |= true;
      } else {
        // Otherwise, continue iterating.
        ++it;
      }
    }

    return modified;
  }

  bool set_difference(const Self &other) {

    bool modified = false;

    // Remove all elements that occur in other.
    auto oit = other.cbegin(), oie = other.cend();

    for (auto it = this->begin(); it != this->end(); ++oit) {

      auto [lower, upper] = std::equal_range(it, this->end(), *oit, Cmp{});

      if (lower != upper) {
        // If the value is in the range, remove it.
        it = this->Base::erase(lower, upper);
        modified |= true;
      } else {
        // Otherwise, continue iterating.
        it = upper;
      }
    }

    return modified;
  }
};

} // namespace memoir

#endif // MEMOIR_SUPPORT_SORTEDVECTOR_H
