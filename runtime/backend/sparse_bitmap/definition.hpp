#ifndef MEMOIR_BACKEND_SPARSEBITMAP_H
#define MEMOIR_BACKEND_SPARSEBITMAP_H

#include <cstdint>
#include <cstdio>
#include <functional>
#include <type_traits>

#include <absl/container/btree_map.h>

using Size = size_t;

template <typename Val, const Size ChunkSize>
struct SparseBitSetChunk {
  std::bitset<ChunkSize> bits;
  std::array<Val, ChunkSize> vals;
};

template <typename Key, typename Val, const Size ChunkSize = 1024>
struct SparseBitMap : absl::btree_map<Size, SparseBitSetChunk<Val, ChunkSize>> {
  using Chunk = typename SparseBitMapChunk<Val, ChunkSize>;
  using Base = typename absl::btree_map<Size, Chunk>;

  SparseBitMap() : Base() {}
  SparseBitMap(const SparseBitMap<Key, Val, ChunkSize> &other) : Base(other) {}
  ~SparseBitMap() {
    // TODO: if the element is a collection pointer, delete it too.
  }

  Chunk &chunk(const Size &key) {
    return this->Base::operator[](key / ChunkSize);
  }

  const Chunk &chunk(const Size &key) const {
    return this->Base::at(key / ChunkSize);
  }

  const Val &read(const Size &key) const {
    return this->chunk(key).vals[key % ChunkSize];
  }

  void write(const Size &key, const Val &val) {
    this->chunk(key).vals[key % ChunkSize] = val;
  }

  Val *get(const Size &key) {
    return &this->chunk(key).vals[key % ChunkSize];
  }

  void insert(const Size &key) {
    this->chunk(key).bits.set(key % ChunkSize);
  }

  void insert(const Size &key, const Val &val) {
    auto &chunk = this->chunk(key);
    auto offset = key % ChunkSize;
    chunk.bits.set(offset);
    chunk.vals[offset] = val;
  }

  void remove(const Size &key) {
    this->chunk(key).bits.reset(key % ChunkSize);
  }

  SparseBitMap<Key, Val, ChunkSize> *copy() {
    // TODO: if Val is a collection type, we need to deep copy.
    auto *copy = new SparseBitMap<Key, Val, ChunkSize>(*this);

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

    return found->second.bits.test(key % ChunkSize);
  }

  size_t size() {
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

    void find_next() {
      for (; this->_it != this->_ie; ++this->_it, ++this->_i) {
        auto &chunk = this->_it->second;

        for (; this->_i < ChunkSize; ++this->_j) {
          if (chunk.bits.test(this->_j)) {
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
      this->_val = into_primitive(this->_it->second.vals[this->_i]);

      // Iterate until we find the next set bit.
      ++this->_i;
      this->find_next();

      return true;
    }
  };

  void begin(iterator *iter) {
    iter->_i = 0;
    iter->_j = 0;
    iter->_it = this->Base::begin();
    iter->_ie = this->Base::end();
  }

  using Base::begin;
  using Base::end;
};

#endif // MEMOIR_BACKEND_SPARSEBITMAP_H
