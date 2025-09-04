#ifndef MEMOIR_BACKEND_SPARSEBITSET_H
#define MEMOIR_BACKEND_SPARSEBITSET_H

#include <bitset>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <type_traits>

#include <roaring/roaring.hh>

template <typename Key>
struct SparseBitSet : roaring::Roaring {
  using Size = size_t;
  using Base = roaring::Roaring;

  SparseBitSet() : Base() {}
  SparseBitSet(const SparseBitSet<Key> &other) : Base(other) {}
  ~SparseBitSet() {
    // TODO: if the element is a collection pointer, delete it too.
  }

  ALWAYS_INLINE void insert(const Size &key) {
    this->Base::add(key);
  }

  ALWAYS_INLINE void insert_input(SparseBitSet<Key> *other) {
    *this |= *other;
  }

  ALWAYS_INLINE void remove(const Size &key) {
    this->Base::remove(key);
  }

  ALWAYS_INLINE SparseBitSet<Key> *copy() {
    // TODO: if Val is a collection type, we need to deep copy.
    auto *copy = new SparseBitSet<Key>(*this);

    return copy;
  }

  ALWAYS_INLINE void clear() {
    this->Base::clear();
  }

  ALWAYS_INLINE bool has(const Size &key) const {
    return this->Base::contains(key);
  }

  ALWAYS_INLINE size_t size() const {
    return this->Base::cardinality();
  }

  struct iterator {
    Size _key;
    Base::const_iterator _it;
    Base::const_iterator _ie;

    iterator(Base &base) : _it(base.begin()), _ie(base.end()) {}

    ALWAYS_INLINE bool next() {
      if (this->_it >= this->_ie) {
        return false;
      }

      // Compute the current key.
      this->_key = *this->_it;

      // Iterate until we find the next set bit.
      ++this->_it;

      return true;
    }
  };

  ALWAYS_INLINE void begin(iterator *iter) {
    iter->_it = this->Base::begin();
    iter->_ie = this->Base::end();
  }
};

#endif // MEMOIR_BACKEND_SPARSEBITSET_H
