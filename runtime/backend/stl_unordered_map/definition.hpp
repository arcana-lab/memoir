#ifndef MEMOIR_BACKEND_STLUNORDEREDMAP_H
#define MEMOIR_BACKEND_STLUNORDEREDMAP_H

#include <cstdint>
#include <cstdio>
#include <functional>
#include <iostream>
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
    return this->Base::at(key);
  }

  void write(const Key &key, const Val &val) {
    this->Base::at(key) = val;
  }

  void insert(const Key &key) {
    (*this)[key];
  }

  void insert_value(const Key &key, const Val &val) {
    (*this)[key] = val;
  }

  void remove(const Key &key) {
    this->Base::erase(key);
  }

  UnorderedMap<Key, Val> *copy() {
    // TODO: if Val is a collection type, we need to deep copy.
    auto *copy = new UnorderedMap<Key, Val>(*this);

    return copy;
  }

  void clear() {
    this->Base::clear();
  }

  bool has(const Key &key) {
    return this->Base::count(key) > 0;
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
    as_primitive_t<Val> _val;
    Base::iterator _it;
    Base::iterator _ie;

    iterator(Base &base) : _it(base.begin()), _ie(base.end()) {}

    bool next() {
      if (this->_it == this->_ie) {
        return false;
      }
      this->_key = this->_it->first;
      this->_val = into_primitive(this->_it->second);
      this->_it = std::next(this->_it);
      return true;
    }
  };

  void begin(iterator *iter) {
    iter->_it = this->Base::begin();
    iter->_ie = this->Base::end();
  }
};

#endif // MEMOIR_BACKEND_STLUNORDEREDMAP_H
