#ifndef MEMOIR_BACKEND_BITSET_H
#define MEMOIR_BACKEND_BITSET_H

#include <cstdint>
#include <cstdio>
#include <functional>
#include <type_traits>

#include <boost/dynamic_bitset.hpp>

#include <backend/stl_vector/definition.hpp>

#ifndef BITSET_TRACK_SIZE
#  define BITSET_TRACK_SIZE 0
#endif

template <typename Key>
struct BitSet : boost::dynamic_bitset<> {
  using Base = typename boost::dynamic_bitset<>;
  using Size = size_t;
  using SizeDiff = ptrdiff_t;

#if BITSET_TRACK_SIZE
  Size _size;
  bool _size_valid;
#endif

  BitSet()
    : Base()
#if BITSET_TRACK_SIZE
      ,
      _size(0)
#endif
  {
  }
  BitSet(const BitSet<Key> &other)
    : Base(other)
#if BITSET_TRACK_SIZE
      ,
      _size(other._size)
#endif
  {
  }
  ~BitSet() {
    // TODO: if the element is a collection pointer, delete it too.
  }

  void increment_size(SizeDiff delta = 1) {
    if (this->_size_valid) {
      this->_size += delta;
    }
  }

  void invalidate_size() {
    this->_size_valid = false;
  }

  void insert(const Size &key) {
    if (this->Base::size() <= key) {
      this->Base::resize(key + 1);
    }

#if BITSET_TRACK_SIZE
    if (not this->Base::test_set(key)) {
      this->increment_size();
    }
#else
    this->Base::set(key);
#endif
  }

  void insert_input(BitSet<Key> *other) {
    auto size = this->Base::size();
    auto other_size = other->Base::size();
    if (size == 0) {
      // If we are uninitialized, just copy.
      *this = *other;
    } else if (size <= other_size) {
      if (size < other_size) {
        this->resize(other_size);
      }

      *this |= *other;
#if BITSET_TRACK_SIZE
      this->invalidate_size();
#endif

    } else {
      Size i = other->find_first();
      while (i != Base::npos) {

#if BITSET_TRACK_SIZE
        if (not this->Base::test_set(i)) {
          this->increment_size();
        }
#else
        this->Base::set(i);
#endif

        i = other->find_next(i);
      }
    }
  }

  void remove(const Size &key) {
    if (key < this->Base::size()) {

#if BITSET_TRACK_SIZE
      if (this->Base::test_set(key, false)) {
        this->increment_size(-1);
      }
#else
      this->Base::reset(key);
#endif
    }
  }

  BitSet<Key> *copy() {
    // TODO: if Val is a collection type, we need to deep copy.
    auto *copy = new BitSet<Key>(*this);

    return copy;
  }

  void clear() {
    this->Base::clear();
#if BITSET_TRACK_SIZE
    this->_size = 0;
    this->_size_valid = true;
#endif
  }

  bool has(const Size &key) {
    if (key < this->Base::size()) {
      return this->Base::test(key);
    }
    return false;
  }

  size_t size() {
#if BITSET_TRACK_SIZE
    if (this->_size_valid) {
      return this->_size;
    }
#endif
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
