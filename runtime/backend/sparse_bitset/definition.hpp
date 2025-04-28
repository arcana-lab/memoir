#ifndef MEMOIR_BACKEND_SPARSEBITSET_H
#define MEMOIR_BACKEND_SPARSEBITSET_H

#include <cstdint>
#include <cstdio>
#include <functional>
#include <type_traits>

#include <absl/container/btree_map.h>

using Size = size_t;

template <typename Key, const size_t ChunkSize = 1024>
struct SparseBitSet : absl::btree_map<Size, std::bitset<ChunkSize>> {
  using Base = typename absl::btree_map<Size, std::bitset<ChunkSize>>;

  SparseBitSet() : Base() {}
  SparseBitSet(const SparseBitSet<Key> &other) : Base(other) {}
  ~SparseBitSet() {
    // TODO: if the element is a collection pointer, delete it too.
  }

  std::bitset<ChunkSize> &chunk(const Size &key) {
    return this->Base::operator[](key / ChunkSize);
  }

  const std::bitset<ChunkSize> &chunk(const Size &key) const {
    return this->Base::at(key / ChunkSize);
  }

  void insert(const Size &key) {
    this->chunk(key).set(key % ChunkSize);
  }

  void insert_input(SparseBitSet<Key> *other) {
    for (const auto &[hi, chunk] : *other) {
      this->chunk(hi) |= chunk;
    }
  }

  void remove(const Size &key) {
    this->chunk(key).reset(key % ChunkSize);
  }

  SparseBitSet<Key> *copy() {
    // TODO: if Val is a collection type, we need to deep copy.
    auto *copy = new SparseBitSet<Key>(*this);

    return copy;
  }

  void clear() {
    this->Base::clear();
  }

  bool has(const Size &key) {
    auto found = this->Base::find(key / ChunkSize);
    if (found == this->Base::end()) {
      return false;
    }

    return found->second.test(key % ChunkSize);
  }

  size_t size() {
    size_t count = 0;
    for (const auto &[hi, chunk] : *this) {
      count += chunk.count();
    }
    return count;
  }

  struct iterator {
    Size _key;
    Size _i, _j;
    SparseBitSet<Key>::Base::iterator _it;
    SparseBitSet<Key>::Base::iterator _ie;

    void find_next() {
      for (; this->_it != this->_ie; ++this->_it, ++this->_i) {
        for (; this->_i < ChunkSize; ++this->_j) {
          if (this->_it->second.test(this->_j)) {
            return;
          }
        }
        this->_j = 0;
      }
    }

    bool next() {
      if (this->_it == this->_ie) {
        return false;
      }

      // Compute the current key.
      this->_key = (this->_i * ChunkSize) + this->_j;

      // Iterate until we find the next set bit.
      ++this->_j;
      this->find_next();

      return true;
    }
  };

  void begin(iterator *iter) {
    iter->_i = 0;
    iter->_j = 0;
    iter->_it = this->begin();
    iter->_ie = this->end();
    iter->find_next();
  }

  using Base::begin;
  using Base::end;
};

#endif // MEMOIR_BACKEND_SPARSEBITSET_H
