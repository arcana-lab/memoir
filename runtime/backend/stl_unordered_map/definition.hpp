#ifndef MEMOIR_BACKEND_STLUNORDEREDMAP_H
#define MEMOIR_BACKEND_STLUNORDEREDMAP_H

#include <cstdint>
#include <cstdio>
#include <functional>
#include <type_traits>

#include <unordered_map>

#include <backend/stl_vector/definition.hpp>

template <typename Key, typename Val>
struct UnorderedMap : std::unordered_map<Key, Val> {
  using Base = typename std::unordered_map<Key, Val>;

  UnorderedMap() : Base() {}
  UnorderedMap(const UnorderedMap<Key, Val> &other) : Base(other) {}
  ~UnorderedMap() {
    // TODO: if the element is a collection pointer, delete it too.
  }

  Val *get(const Key &key) {
    return &(*this)[key];
  }

  Val read(const Key &key) {
    return this->at(key);
  }

  void write(const Key &key, const Val &val) {
    this->at(key) = val;
  }

  void insert(const Key &key) {
    (*this)[key];
  }

  void insert(const Key &key, const Val &val) {
    (*this)[key] = val;
  }

  void remove(const Key &key) {
    this->erase(key);
  }

  UnorderedMap<Key, Val> *copy() {
    // TODO: if Val is a collection type, we need to deep copy.
    auto *copy = new UnorderedMap<Key, Val>(*this);

    return copy;
  }

  void clear() {
    this->clear();
  }

  bool has(const Key &key) {
    return this->count(key) > 0;
  }

  size_t size() {
    return this->Base::size();
  }

  Vector<Key> *keys() {
    auto *keys = new Vector<Key>(this->size());
    size_t i = 0;
    for (const auto &[key, _] : *this) {
      (*keys)[i++] = key;
    }
    return keys;
  }

  struct iterator {
    Key _key;
    Val _val;
    Base::iterator _it;
    Base::iterator _ie;

    bool next() {
      if (this->_it == this->_ie) {
        return false;
      }
      auto [key, val] = *this->_it;
      this->_key = key;
      this->_val = val;
      this->_it = std::next(this->_it);
      return true;
    }
  };

  using Base::begin;
  using Base::end;

  void begin(iterator *iter) {
    iter->_it = this->begin();
    iter->_ie = this->end();
  }
};

#endif // MEMOIR_BACKEND_STLUNORDEREDMAP_H
