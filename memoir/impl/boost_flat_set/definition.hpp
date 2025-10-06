#ifndef MEMOIR_BACKEND_BOOSTFLATSET_H
#define MEMOIR_BACKEND_BOOSTFLATSET_H

#include <cstdint>
#include <cstdio>
#include <functional>
#include <type_traits>

#include <boost/container/flat_set.hpp>

#include <memoir/impl/stl_vector/definition.hpp>

template <typename Key>
struct FlatSet : boost::container::flat_set<Key> {
  using Base = typename boost::container::flat_set<Key>;

  FlatSet() : Base() {}
  FlatSet(const FlatSet<Key> &other) : Base(other) {}
  ~FlatSet() {
    // TODO: if the element is a collection pointer, delete it too.
  }

  void insert(const Key &key) {
    this->Base::insert(key);
  }

  void insert_input(FlatSet<Key> *other) {
    this->Base::insert(boost::container::ordered_unique_range,
                       other->begin(),
                       other->end());
  }

  void remove(const Key &key) {
    this->Base::erase(key);
  }

  FlatSet<Key> *copy() {
    // TODO: if Val is a collection type, we need to deep copy.
    auto *copy = new FlatSet<Key>(*this);

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
    Base::iterator _it;
    Base::iterator _ie;

    iterator(Base &base) : _it(base.begin()), _ie(base.end()) {}

    bool next() {
      if (this->_it == this->_ie) {
        return false;
      }
      this->_key = *this->_it;
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

#endif // MEMOIR_BACKEND_BOOSTFLATSET_H
