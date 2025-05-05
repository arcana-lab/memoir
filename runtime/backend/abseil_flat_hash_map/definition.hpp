#ifndef MEMOIR_BACKEND_ABSEILFLATHASHMAP_H
#define MEMOIR_BACKEND_ABSEILFLATHASHMAP_H

#include <cstdint>
#include <cstdio>
#include <functional>
#include <type_traits>

#include <absl/container/flat_hash_map.h>

template <typename Key, typename Val>
struct FlatHashMap : absl::flat_hash_map<Key, Val> {
  using Base = typename absl::flat_hash_map<Key, Val>;

  FlatHashMap() : Base() {}
  FlatHashMap(const FlatHashMap<Key, Val> &other) : Base(other) {}
  ~FlatHashMap() {
    // TODO: if the element is a collection pointer, delete it too.
  }

  ALWAYS_INLINE const Val &read(const Key &key) {
    try {
      return this->Base::at(key);
    } catch (const std::out_of_range &exc) {
      errorf("FlatHashMap: Out of bounds at %zu\n", key);
      exit(11);
    }
  }

  ALWAYS_INLINE void write(const Key &key, const Val &val) {
    try {
      this->Base::at(key) = val;
    } catch (const std::out_of_range &exc) {
      errorf("FlatHashMap: Out of bounds at %zu\n", key);
      exit(11);
    }
  }

  ALWAYS_INLINE Val *get(const Key &key) {
    try {
      return &this->Base::at(key);
    } catch (const std::out_of_range &exc) {
      errorf("FlatHashMap: Out of bounds at %zu\n", key);
      exit(11);
    }
  }

  ALWAYS_INLINE void insert(const Key &key) {
    this->Base::emplace(key, Val());
  }

  ALWAYS_INLINE void insert_value(const Key &key, const Val &val) {
    this->Base::emplace(key, val);
  }

  ALWAYS_INLINE void remove(const Key &key) {
    this->Base::erase(key);
  }

  ALWAYS_INLINE FlatHashMap<Key, Val> *copy() {
    // TODO: if Val is a collection type, we need to deep copy.
    auto *copy = new FlatHashMap<Key, Val>(*this);

    return copy;
  }

  ALWAYS_INLINE void clear() {
    this->Base::clear();
  }

  ALWAYS_INLINE bool has(const Key &key) {
    return this->Base::contains(key);
  }

  ALWAYS_INLINE size_t size() {
    return this->Base::size();
  }

  struct iterator {
    Key _key;
    as_primitive_t<Val> _val;
    Base::iterator _it;
    Base::iterator _ie;

    ALWAYS_INLINE bool next() {
      if (this->_it == this->_ie) {
        return false;
      }

      this->_key = this->_it->first;
      this->_val = into_primitive(this->_it->second);

      this->_it = std::next(this->_it);

      return true;
    }
  };

  using Base::begin;
  using Base::end;

  ALWAYS_INLINE void begin(iterator *iter) {
    iter->_it = this->begin();
    iter->_ie = this->end();
  }
};

#endif // MEMOIR_BACKEND_ABSEILFLATHASHMAP_H
