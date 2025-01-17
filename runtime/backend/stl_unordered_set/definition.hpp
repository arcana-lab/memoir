#ifndef MEMOIR_BACKEND_STLUNORDEREDSET_H
#define MEMOIR_BACKEND_STLUNORDEREDSET_H

#include <cstdint>
#include <cstdio>
#include <functional>
#include <type_traits>

#include <unordered_set>

#include <backend/stl_vector/definition.hpp>

template <typename Key>
struct UnorderedSet : std::unordered_set<Key> {
  UnorderedSet() : std::unordered_set<Key>() {}
  UnorderedSet(const UnorderedSet<Key> &other)
    : std::unordered_set<Key>(other) {}
  ~UnorderedSet() {
    // TODO: if the element is a collection pointer, delete it too.
  }

  void insert(const Key &key) {
    this->std::unordered_set<Key>::insert(key);
  }

  void remove(const Key &key) {
    this->erase(key);
  }

  UnorderedSet<Key> *copy() {
    // TODO: if Val is a collection type, we need to deep copy.
    auto *copy = new UnorderedSet<Key>(*this);

    return copy;
  }

  void clear() {
    this->clear();
  }

  bool has(const Key &key) {
    return this->count(key) > 0;
  }

  size_t size() {
    return this->size();
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
    std::unordered_set<Key>::iterator _it;
    std::unordered_set<Key>::iterator _ie;

    bool next() {
      if (this->_it == this->_ie) {
        return false;
      }
      this->_key = *this->_it;
      this->_it = std::next(this->_it);
      return true;
    }
  };

  using std::unordered_set<Key>::begin;
  using std::unordered_set<Key>::end;

  void begin(iterator *iter) {
    iter->_it = this->begin();
    iter->_ie = this->end();
  }
};

#endif // MEMOIR_BACKEND_STLUNORDEREDSET_H
