#ifndef MEMOIR_BACKEND_SPARSEBITSET_H
#define MEMOIR_BACKEND_SPARSEBITSET_H

#include <bitset>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <type_traits>

#include <absl/container/btree_map.h>

using Size = size_t;

template <typename Key, const size_t ChunkSize = 256>
struct SparseBitSet : absl::btree_map<Size, std::bitset<ChunkSize>> {
  using Base = typename absl::btree_map<Size, std::bitset<ChunkSize>>;

  SparseBitSet() : Base() {}
  SparseBitSet(const SparseBitSet<Key> &other) : Base(other) {}
  ~SparseBitSet() {
    // TODO: if the element is a collection pointer, delete it too.
  }

  ALWAYS_INLINE std::bitset<ChunkSize> &chunk(const Size &key) {
    return this->Base::operator[](key / ChunkSize);
  }

  ALWAYS_INLINE const std::bitset<ChunkSize> &chunk(const Size &key) const {
    return this->Base::at(key / ChunkSize);
  }

  ALWAYS_INLINE void insert(const Size &key) {
    this->chunk(key).set(key % ChunkSize);
  }

  ALWAYS_INLINE void insert_input(SparseBitSet<Key> *other) {
    for (const auto &[hi, chunk] : *other) {
      this->chunk(hi) |= chunk;
    }
  }

  ALWAYS_INLINE void remove(const Size &key) {
    this->chunk(key).reset(key % ChunkSize);
  }

  ALWAYS_INLINE SparseBitSet<Key> *copy() {
    // TODO: if Val is a collection type, we need to deep copy.
    auto *copy = new SparseBitSet<Key>(*this);

    return copy;
  }

  ALWAYS_INLINE void clear() {
    this->Base::clear();
  }

  ALWAYS_INLINE bool has(const Size &key) {
    auto found = this->Base::find(key / ChunkSize);
    if (found == this->Base::end()) {
      return false;
    }

    return found->second.test(key % ChunkSize);
  }

  ALWAYS_INLINE size_t size() {
    size_t count = 0;
    for (const auto &[hi, chunk] : *this) {
      count += chunk.count();
    }
    return count;
  }

  struct iterator {
    Size _key;
    Size _j;
    SparseBitSet<Key>::Base::iterator _it;
    SparseBitSet<Key>::Base::iterator _ie;

    ALWAYS_INLINE void find_next() {
      for (; this->_it != this->_ie; ++this->_it) {
        for (; this->_j < ChunkSize; ++this->_j) {
          if (this->_it->second.test(this->_j)) {
            return;
          }
        }
        this->_j = 0;
      }
    }

    ALWAYS_INLINE bool next() {
      if (this->_it == this->_ie) {
        return false;
      }

      // Compute the current key.
      this->_key = (this->_it->first * ChunkSize) + this->_j;

      // Iterate until we find the next set bit.
      ++this->_j;
      this->find_next();

      return true;
    }
  };

  ALWAYS_INLINE void begin(iterator *iter) {
    iter->_j = 0;
    iter->_it = this->begin();
    iter->_ie = this->end();
    iter->find_next();
  }

  using Base::begin;
  using Base::end;
};

#endif // MEMOIR_BACKEND_SPARSEBITSET_H
