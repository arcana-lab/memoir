#ifndef MEMOIR_SUPPORT_RANGEMAP_H
#define MEMOIR_SUPPORT_RANGEMAP_H

#include "memoir/support/DataTypes.hpp"

namespace memoir {

template <typename Key,
          typename Val,
          typename KeyCmp = std::less<Key>,
          typename ValCmp = std::less<Val>>
struct UniqueMultiMap : public OrderedMultiMap<Key, Val, KeyCmp> {
  using Base = OrderedMultiMap<Key, Val, KeyCmp>;
  using Iter = Base::iterator;
  using ConstIter = Base::const_iterator;

  /**
   * Tries to insert the key value pair.
   * @return the iterator and a boolean, which is true if a new value was
   * inserted and false otherwise.
   */
  Pair<Iter, bool> insert(const Key &key, const Val &val) {
    auto [lo, hi] = this->Base::equal_range(key);

    ValCmp cmp{};
    auto it = lo;
    for (; it != hi; ++it) {
      if (cmp(val, it->second)) {
        // val < curr => not present.
        break;
      } else if (cmp(it->second, val)) {
        // curr < val => keep searching.
        continue;
      } else {
        // curr == val => found!
        return Pair<Iter, bool>(it, false);
      }
    }

    it = this->Base::emplace_hint(it, key, val);

    return Pair<Iter, bool>(it, true);
  }

  /**
   * Inserts unique values from the input range.
   */
  void insert(std::input_iterator auto begin, std::input_iterator auto end) {
    for (auto it = begin; it != end; ++it) {
      this->insert(it->first, it->second);
    }
  }
};

} // namespace memoir

#endif // MEMOIR_SUPPORT_RANGEMAP_H
