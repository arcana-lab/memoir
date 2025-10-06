#ifndef MEMOIR_BACKEND_STLMULTIMAP_H
#define MEMOIR_BACKEND_STLMULTIMAP_H

#include <cstdint>
#include <cstdio>
#include <functional>
#include <type_traits>

#include <boost/container/flat_map.hpp>

#include <memoir/impl/stl_vector/definition.hpp>

template <typename Key, typename Val>
struct FlatMultiMap : boost::container::flat_multimap<Key, Val> {
  using Base = typename boost::container::flat_multimap<Key, Val>;

  FlatMultiMap() : Base() {}
  FlatMultiMap(const FlatMultiMap<Key, Val> &other) : Base(other) {}
  ~FlatMultiMap() {
    // TODO: if the element is a collection pointer, delete it too.
  }

  Val *get(const Key &key, const size_t &index) {
    return &std::next(this->lower_bound(key), index)->second;
  }

  Val read(const Key &key, const size_t &index) {
    return *this->get(key, index);
  }

  void write(const Key &key, const size_t &index, const Val &val) {
    *this->get(key, index) = val;
  }

  static Val default_value() {
    if constexpr (std::is_pointer_v<Val>) {
      return nullptr;
    } else {
      return Val();
    }
  }

  using Base::insert;

  void insert(const Key &key, const size_t &index) {
    this->insert(std::next(this->lower_bound(key), index),
                 std::make_pair(key, default_value()));
  }

  void insert(const Key &key, const size_t &index, const Val &val) {
    this->insert(std::next(this->lower_bound(key), index),
                 std::make_pair(key, val));
  }

  void insert(const Key &key,
              const size_t &start,
              std::input_iterator auto begin,
              std::input_iterator auto end) {
    this->insert(std::next(this->lower_bound(key), start), begin, end);
  }

  void insert(const Key &key) {
    // TODO
  }

  void remove(const Key &key, const size_t &index) {
    this->erase(std::next(this->lower_bound(key), index));
  }

  void remove(const Key &key, const size_t &begin, const size_t &end) {
    this->erase(std::next(this->lower_bound(key), begin),
                std::next(this->lower_bound(key), end));
  }

  void remove(const Key &key) {
    this->erase(this->lower_bound(key), this->upper_bound(key));
  }

  FlatMultiMap<Key, Val> *copy() {
    // TODO: if Val is a collection type, we need to deep copy.
    auto *copy = new FlatMultiMap<Key, Val>(*this);

    return copy;
  }

  void clear(const Key &key) {
    this->erase(this->lower_bound(key), this->upper_bound(key));
  }

  void clear() {
    this->clear();
  }

  bool has(const Key &key) {
    return this->count(key) > 0;
  }

  size_t size(const Key &key) {
    return std::distance(this->lower_bound(key), this->upper_bound(key));
  }

  size_t size() {
    size_t n = 0;
    for (auto it = this->begin(); it != this->end();
         it = this->upper_bound(it->first)) {
      ++n;
    }
    return n;
  }

  Vector<Key> *keys() {
    auto *keys = new Vector<Key>(this->size());
    size_t i = 0;
    for (auto it = this->begin(); it != this->end();
         it = this->upper_bound(it->first)) {
      keys->write(i, it->first);
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

  // TODO: inner iterator
};

#endif // MEMOIR_BACKEND_STLMULTIMAP_H
