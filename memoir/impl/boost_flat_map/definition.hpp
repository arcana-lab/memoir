#ifndef MEMOIR_BACKEND_BOOSTFLATMAP_H
#define MEMOIR_BACKEND_BOOSTFLATMAP_H

#include <cstdint>
#include <cstdio>
#include <functional>
#include <type_traits>

#include <boost/container/flat_map.hpp>

template <typename Key, typename Val>
struct FlatMap : boost::container::flat_map<Key, Val> {
  using Base = typename boost::container::flat_map<Key, Val>;

  FlatMap() : Base() {}
  FlatMap(const FlatMap<Key, Val> &other) : Base(other) {}
  ~FlatMap() {
    // TODO: if the element is a collection pointer, delete it too.
  }

  const Val &read(const Key &key) {
    return this->Base::at(key);
  }

  void write(const Key &key, const Val &val) {
    this->Base::at(key) = val;
  }

  Val *get(const Key &key) {
    return &this->Base::at(key);
  }

  void insert(const Key &key) {
    this->Base::operator[](key);
  }

  void insert_value(const Key &key, const Val &val) {
    this->Base::emplace(key, val);
  }

  void remove(const Key &key) {
    this->Base::erase(key);
  }

  FlatMap<Key, Val> *copy() {
    // TODO: if Val is a collection type, we need to deep copy.
    auto *copy = new FlatMap<Key, Val>(*this);

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

  struct iterator {
    Key _key;
    as_primitive_t<Val> _val;
    Base::iterator _it;
    Base::iterator _ie;

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

  using Base::begin;
  using Base::end;

  void begin(iterator *iter) {
    iter->_it = this->begin();
    iter->_ie = this->end();
  }
};

#endif // MEMOIR_BACKEND_BOOSTFLATMAP_H
