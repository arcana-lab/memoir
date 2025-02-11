#ifndef MEMOIR_BACKEND_BITSET_H
#define MEMOIR_BACKEND_BITSET_H

#include <cstdint>
#include <cstdio>
#include <functional>
#include <type_traits>

#include <boost/dynamic_bitset.hpp>

#include <backend/stl_vector/definition.hpp>

template <typename Key>
struct BitSet : boost::dynamic_bitset<> {
  using Base = typename boost::dynamic_bitset<>;

  using Size = size_t;

  BitSet() : Base() {}
  BitSet(const BitSet<Key> &other) : Base(other) {}
  ~BitSet() {
    // TODO: if the element is a collection pointer, delete it too.
  }

  void insert(const Size &key) {
    if (this->Base::size() <= key) {
      this->Base::resize(key + 1);
    }
    this->Base::set(key);
  }

  void insert_input(BitSet<Key> *other) {
    auto size = std::min(this->size(), other->size());
#pragma clang loop vectorize(enable)
    for (Size i = 0; i < size; ++i) {
      (*this)[i] |= (*other)[i];
    }
  }

  void remove(const Size &key) {
    if (key < this->Base::size()) {
      this->Base::reset(key);
    }
  }

  BitSet<Key> *copy() {
    // TODO: if Val is a collection type, we need to deep copy.
    auto *copy = new BitSet<Key>(*this);

    return copy;
  }

  void clear() {
    this->Base::clear();
  }

  bool has(const Size &key) {
    return this->Base::test(key);
  }

  size_t size() {
    return this->Base::count();
  }

  Vector<Size> *keys() {
    auto *keys = new Vector<Size>(this->size());
    // TODO
    return keys;
  }

  struct iterator {
    Size _key;
    Size _cur;
    BitSet<Key> *_set;

    bool next() {
      if (this->_cur == Base::npos) {
        return false;
      }
      this->_key = this->_cur;
      this->_cur = this->_set->find_next(this->_cur);
      return true;
    }
  };

  void begin(iterator *iter) {
    iter->_cur = this->find_first();
    iter->_set = this;
  }
};

#endif // MEMOIR_BACKEND_BITSET_H
