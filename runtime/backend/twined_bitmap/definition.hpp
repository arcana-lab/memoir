#ifndef MEMOIR_BACKEND_BITMAP_H
#define MEMOIR_BACKEND_BITMAP_H

#include <array>
#include <bitset>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <type_traits>

#include <backend/stl_vector/definition.hpp>

template <typename Val, const Size ChunkSize>
struct TwinedBitMapChunk {
  std::bitset<ChunkSize> bits;
  std::array<Val, ChunkSize> vals;

  ALWAYS_INLINE Size base(const Size &i) const {
    return i / ChunkSize;
  }

  ALWAYS_INLINE Size offset(const Size &i) const {
    return i % ChunkSize;
  }

  Size count() const {
    return this->bits.count();
  }

  void set(const Size &key) {
    this->bits.set(offset(key));
  }

  void reset(const Size &key) {
    this->bits.reset(offset(key));
  }

  bool test(const Size &key) const {
    return this->bits.test(offset(key));
  }

  Val &val(const Size &key) {
    return this->vals[offset(key)];
  }

  const Val &val(const Size &key) const {
    return this->vals[offset(key)];
  }
};

template <typename Key, typename Val, const Size ChunkSize = 256>
struct TwinedBitMap : std::vector<TwinedBitMapChunk<Val, ChunkSize>> {
  using Base = typename std::vector<TwinedBitMapChunk<Val, ChunkSize>>;
  using Chunk = TwinedBitMapChunk<Val, ChunkSize>;

protected:
  ALWAYS_INLINE Size base(const Size &i) const {
    return i / ChunkSize;
  }

  ALWAYS_INLINE Size offset(const Size &i) const {
    return i % ChunkSize;
  }

  ALWAYS_INLINE Chunk &chunk(const Size &i) {
    return this->Base::operator[](base(i));
  }

  ALWAYS_INLINE const Chunk &chunk(const Size &i) const {
    return this->Base::at(base(i));
  }

  ALWAYS_INLINE void resize(const Size &key) {
    if (this->Base::size() <= base(key)) {
      this->Base::resize(base(key) + 1);
    }
  }

  ALWAYS_INLINE bool inbounds(const Size &key) {
    return base(key) < this->Base::size();
  }

public:
  TwinedBitMap() : Base() {}
  TwinedBitMap(const TwinedBitMap<Key, Val> &other) : Base(other) {}
  ~TwinedBitMap() {
    // TODO: if the element is a collection pointer, delete it too.
  }

  Val read(const Size &key) {
    return this->chunk(key).val(key);
  }

  template <typename T = Val,
            typename std::enable_if_t<!std::is_same_v<T, bool>, bool> = 0>
  Val *get(const Size &key) {
    return &this->chunk(key).val(key);
  }

  template <typename T = Val,
            typename std::enable_if_t<std::is_same_v<T, bool>, bool> = 0>
  Val *get(const Size &key) {
    return nullptr;
  }

  void write(const Size &key, Val val) {
    this->chunk(key).val(key) = val;
  }

  void insert(const Size &key) {
    this->resize(key);
    this->chunk(key).set(key);
  }

  void insert_value(const Size &key, const Val &val) {
    this->resize(key);

    auto &chunk = this->chunk(key);
    chunk.set(key);
    chunk.val(key) = val;
  }

  void remove(const Size &key) {
    if (this->inbounds(key)) {
      this->chunk(key).reset(key);
    }
  }

  TwinedBitMap<Key, Val> *copy() {
    // TODO: if Val is a collection type, we need to deep copy.
    auto *copy = new TwinedBitMap<Key, Val>(*this);

    return copy;
  }

  void clear() {
    this->Base::clear();
  }

  bool has(const Size &key) {
    if (this->inbounds(key)) {
      return this->chunk(key).test(key);
    }
    return false;
  }

  size_t size() {
    size_t count = 0;
    for (const auto &chunk : *this) {
      count += chunk.count();
    }
    return count;
  }

  struct iterator {
    Size _key;
    as_primitive_t<Val> _val;
    Base::iterator _it, _ie;
    Size _i, _j;

    void find_next() {
      for (; this->_it != this->_ie; ++this->_it, ++this->_i) {
        auto &chunk = *this->_it;

        for (; this->_j < ChunkSize; ++this->_j) {
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
      this->_val = into_primitive(this->_it->val(this->_j));

      // Iterate until we find the next set bit.
      ++this->_j;
      this->find_next();

      return true;
    }
  };

  void begin(iterator *iter) {
    iter->_it = this->begin();
    iter->_ie = this->end();
    iter->_i = 0;
    iter->_j = 0;
    iter->find_next();
  }

  using Base::begin;
  using Base::end;
};

#endif // MEMOIR_BACKEND_BITSET_H
