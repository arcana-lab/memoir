#ifndef MEMOIR_BACKEND_ABSEILFLATHASHSET_H
#define MEMOIR_BACKEND_ABSEILFLATHASHSET_H

#include <cstdint>
#include <cstdio>
#include <functional>
#include <type_traits>

#include <absl/container/flat_hash_set.h>

template <typename Key>
struct FlatHashSet : absl::flat_hash_set<Key> {
  using Base = typename absl::flat_hash_set<Key>;

  FlatHashSet() : Base() {}
  FlatHashSet(const FlatHashSet<Key> &other) : Base(other) {}
  ~FlatHashSet() {
    // TODO: if the element is a collection pointer, delete it too.
  }

  ALWAYS_INLINE void insert(const Key &key) {
    this->Base::insert(key);
  }

  ALWAYS_INLINE void insert_input(FlatHashSet<Key> *other) {
    this->Base::insert(other->begin(), other->end());
  }

  ALWAYS_INLINE void remove(const Key &key) {
    this->Base::erase(key);
  }

  ALWAYS_INLINE
  FlatHashSet<Key> *copy() {
    // TODO: if Val is a collection type, we need to deep copy.
    auto *copy = new FlatHashSet<Key>(*this);

    return copy;
  }

  ALWAYS_INLINE
  void clear() {
    this->Base::clear();
  }

  ALWAYS_INLINE
  bool has(const Key &key) {
    return this->Base::count(key) > 0;
  }

  ALWAYS_INLINE
  size_t size() {
    return this->Base::size();
  }

  struct iterator {
    Key _key;
    Base::iterator _it;
    Base::iterator _ie;

    ALWAYS_INLINE
    bool next() {
      if (this->_it == this->_ie) {
        return false;
      }
      this->_key = *this->_it;
      this->_it = std::next(this->_it);
      return true;
    }
  };

  using Base::begin;
  using Base::end;

  ALWAYS_INLINE
  void begin(iterator *iter) {
    iter->_it = this->begin();
    iter->_ie = this->end();
  }
};

#endif // MEMOIR_BACKEND_BOOSTFLATSET_H
