#ifndef MEMOIR_BACKEND_BITMAP_H
#define MEMOIR_BACKEND_BITMAP_H

#include <cstdint>
#include <cstdio>
#include <functional>
#include <type_traits>

#include <boost/dynamic_bitset.hpp>

#include <backend/stl_vector/definition.hpp>

template <typename Key, typename Val>
struct BitMap : boost::dynamic_bitset<> {
  using Base = typename boost::dynamic_bitset<>;
  using Values = Vector<Val>;

  using Size = size_t;

protected:
  Values _vals;

public:
  BitMap() : Base(), _vals{} {}
  BitMap(const BitMap<Key, Val> &other) : Base(other), _vals(other._vals) {}
  ~BitMap() {
    // TODO: if the element is a collection pointer, delete it too.
  }

  void insert(const Size &key) {
    if (this->Base::size() <= key) {
      this->Base::resize(key + 1);
    }
    if (this->_vals.size() <= key) {
      this->_vals.resize(key + 1);
    }

    this->Base::set(key);
  }

  void insert_value(const Size &key, const Val &val) {
    this->insert(key);
    this->_vals[key] = val;
  }

#if 0
  void insert_input(BitMap<Key, Val> *other) {
    auto size = this->Base::size();
    auto other_size = other->Base::size();
    if (size < other_size) {
      this->resize(other_size);
    }
    for (Size i = 0; i < other_size; ++i) {
      (*this)[i] |= (*other)[i];
    }
  }
#endif

  void remove(const Size &key) {
    if (key < this->Base::size()) {
      this->Base::reset(key);
    }
  }

  BitMap<Key, Val> *copy() {
    // TODO: if Val is a collection type, we need to deep copy.
    auto *copy = new BitMap<Key, Val>(*this);

    return copy;
  }

  void clear() {
    this->Base::clear();
    this->_vals.clear();
  }

  bool has(const Size &key) {
    if (key < this->Base::size()) {
      return this->Base::test(key);
    }
    return false;
  }

  Val read(const Size &key) {
    return _vals.read(key);
  }

  template <typename T = Val,
            typename std::enable_if_t<!std::is_same_v<T, bool>, bool> = 0>
  Val *get(const Size &key) {
    return this->_vals.get(key);
  }

  template <typename T = Val,
            typename std::enable_if_t<std::is_same_v<T, bool>, bool> = 0>
  Val *get(const Size &key) {
    return nullptr;
  }

  void write(const Size &key, Val val) {
    this->_vals.write(key, val);
  }

  size_t size() {
    return this->Base::count();
  }

  struct iterator {
    Size _key;
    as_primitive_t<Val> _val;
    Size _cur;
    BitMap<Key, Val> *_map;

    iterator(BitMap<Key, Val> &base) : _map(&base), _cur(base.find_first()) {}

    bool next() {
      if (this->_cur == Base::npos) {
        return false;
      }
      this->_key = this->_cur;
      if constexpr (std::is_same_v<Val, bool>) {
        this->_val = this->_map->read(this->_key);
      } else {
        this->_val = into_primitive(this->_map->_vals[this->_key]);
      }
      this->_cur = this->_map->find_next(this->_cur);
      return true;
    }
  };

  void begin(iterator *iter) {
    iter->_cur = this->find_first();
    iter->_map = this;
  }
};

#endif // MEMOIR_BACKEND_BITSET_H
