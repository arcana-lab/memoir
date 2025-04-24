#ifndef MEMOIR_BACKEND_SPARSEBITMAP_H
#define MEMOIR_BACKEND_SPARSEBITMAP_H

#include <cstdint>
#include <cstdio>
#include <functional>
#include <map>
#include <type_traits>

#include <backend/stl_vector/definition.hpp>

using Size = size_t;

template <typename Val, const Size ChunkSize>
struct Chunk {
  std::bitset<ChunkSize> bits;
  std::array<Val, ChunkSize> vals;
};

template <typename Key, typename Val, const Size ChunkSize = 1024>
struct SparseBitMap : std::map<Size, Chunk<Val, ChunkSize>> {
  using Base = typename std::map<Size, Chunk<Val, ChunkSize>>;

  SparseBitMap() : Base() {}
  SparseBitMap(const SparseBitMap<Key, Val, ChunkSize> &other) : Base(other) {}
  ~SparseBitMap() {
    // TODO: if the element is a collection pointer, delete it too.
  }

  Chunk<Val, ChunkSize> &chunk(const Size &key) {
    return this->Base::operator[](key / ChunkSize);
  }

  const Chunk<Val, ChunkSize> &chunk(const Size &key) const {
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
    Size _i;
    SparseBitMap<Key, Val, ChunkSize>::Base::iterator _it;
    SparseBitMap<Key, Val, ChunkSize>::Base::iterator _ie;

    bool next() {
      if (this->_it == this->_ie) {
        return false;
      }

      // Compute the current key.
      this->_key = (this->_it->first * ChunkSize) + this->_i;
      this->_val = into_primitive(this->_it->second.vals[this->_i]);

      // Iterate until we find the next set bit.
      for (; this->_it != this->_ie; ++this->_it) {
        auto &chunk = this->_it->second;

        for (; this->_i < ChunkSize; ++this->_i) {
          if (chunk.bits.test(this->_i)) {
            return true;
          }
        }
        this->_i = 0;
      }

      // If we go this far, then there are no more set bits.
      return false;
    }
  };

  void begin(iterator *iter) {
    iter->_i = 0;
    iter->_it = this->Base::begin();
    iter->_ie = this->Base::end();
  }

  using Base::begin;
  using Base::end;
};

#endif // MEMOIR_BACKEND_SPARSEBITMAP_H
