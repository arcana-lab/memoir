#ifndef MEMOIR_BACKEND_SPARSEBITMAP_H
#define MEMOIR_BACKEND_SPARSEBITMAP_H

#include <array>
#include <bitset>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <type_traits>

#include <absl/container/btree_map.h>

template <typename Val, const Size ChunkSize>
struct SparseBitMapChunk {
  std::bitset<ChunkSize> bits;
  std::array<Val, ChunkSize> vals;
};

template <typename Key, typename Val, const Size ChunkSize = 256>
struct SparseBitMap : absl::btree_map<Size, SparseBitMapChunk<Val, ChunkSize>> {
  using Chunk = SparseBitMapChunk<Val, ChunkSize>;
  using Base = absl::btree_map<Size, Chunk>;

  SparseBitMap() : Base() {}
  SparseBitMap(const SparseBitMap<Key, Val, ChunkSize> &other) : Base(other) {}
  ~SparseBitMap() {
    // TODO: if the element is a collection pointer, delete it too.
  }

  ALWAYS_INLINE Chunk &chunk(const Size &key) {
    return this->Base::operator[](key / ChunkSize);
  }

  ALWAYS_INLINE const Chunk &chunk(const Size &key) const {
    return this->Base::at(key / ChunkSize);
  }

  ALWAYS_INLINE const Val &read(const Size &key) const {
    return this->chunk(key).vals[key % ChunkSize];
  }

  ALWAYS_INLINE void write(const Size &key, const Val &val) {
    this->chunk(key).vals[key % ChunkSize] = val;
  }

  ALWAYS_INLINE Val *get(const Size &key) {
    return &this->chunk(key).vals[key % ChunkSize];
  }

  ALWAYS_INLINE void insert(const Size &key) {
    this->chunk(key).bits.set(key % ChunkSize);
  }

  ALWAYS_INLINE void insert(const Size &key, const Val &val) {
    auto &chunk = this->chunk(key);
    auto offset = key % ChunkSize;
    chunk.bits.set(offset);
    chunk.vals[offset] = val;
  }

  ALWAYS_INLINE void remove(const Size &key) {
    this->chunk(key).bits.reset(key % ChunkSize);
  }

  ALWAYS_INLINE SparseBitMap<Key, Val, ChunkSize> *copy() {
    // TODO: if Val is a collection type, we need to deep copy.
    auto *copy = new SparseBitMap<Key, Val, ChunkSize>(*this);

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

    return found->second.bits.test(key % ChunkSize);
  }

  ALWAYS_INLINE size_t size() {
    size_t count = 0;
    for (const auto &[hi, chunk] : *this) {
      count += chunk.bits.count();
    }
    return count;
  }

  struct iterator {
    Size _key;
    as_primitive_t<Val> _val;
    Size _i, _j;
    Base::iterator _it;
    Base::iterator _ie;

    ALWAYS_INLINE void find_next() {
      for (; this->_it != this->_ie; ++this->_it) {
        auto &chunk = this->_it->second;

        for (; this->_j < ChunkSize; ++this->_j) {
          if (chunk.bits.test(this->_j)) {
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
      this->_val = into_primitive(this->_it->second.vals[this->_j]);

      // Iterate until we find the next set bit.
      ++this->_j;
      this->find_next();

      return true;
    }
  };

  ALWAYS_INLINE void begin(iterator *iter) {
    iter->_j = 0;
    iter->_it = this->Base::begin();
    iter->_ie = this->Base::end();
  }

  using Base::begin;
  using Base::end;
};

#endif // MEMOIR_BACKEND_SPARSEBITMAP_H
