#ifndef MEMOIR_BACKEND_STLUNORDEREDMAP_H
#define MEMOIR_BACKEND_STLUNORDEREDMAP_H

#include <cstdint>
#include <cstdio>
#include <functional>
#include <type_traits>

#include <boost/dynamic_bitset.hpp>

#include <backend/stl_vector/definition.hpp>

using Size = size_t;

template <typename Key>
struct KeyToIndexMap : std::unordered_map<Key, Size> {
  using Base = std::unordered_map<Key, Size>;

  Size indexof(const Key &key) {
    return this->Base::at(key);
  }

  Size addkey(const Key &key) {
    auto i = this->Base::size();
    this->Base::insert({ key, i });
    return i;
  }

  bool haskey(const Key &key) {
    return this->Base::count(key) > 0;
  }

#if 0
  const Key &keyof(const Size &i) {
    // TODO
  }
#endif
};

template <typename Key, typename Val>
struct IndexMap {
protected:
  // Extend the map to support index i.
  void extend(const Size &i) {
    if (i >= _data.size()) {
      _data.resize(i + 1);
      _present.resize(i + 1);
    }
  }

public:
  using BitSet = boost::dynamic_bitset<>;
  BitSet _present;

  using Data = std::vector<Val>;
  Data _data;

  using ForwardMap = KeyToIndexMap<Key>;
  ForwardMap *_forward;

  // using ReverseMap = IndexToKeyMap<Key>;
  // ReverseMap *_reverse;

  IndexMap() : _present(), _data(), _forward(new ForwardMap()) {}
  IndexMap(ForwardMap *forward) : _present(), _data(), _forward(forward) {}
  IndexMap(const IndexMap<Key, Val> &other)
    : _present(other._present),
      _data(other._data),
      _forward(other._forward) {}
  ~IndexMap() {
    // TODO: if the element is a collection pointer, delete it too.
  }

  // ====================================
  // === IndexMap specific operations ===
  ForwardMap *forward() {
    return _forward;
  }

  Size indexof(const Key &key) {
    return _forward->indexof(key);
  }

#if 0
  const Key &keyof(const Size &index) {
    return _forward->keyof(index);
  }
#endif
  // ====================================

  Val *get_index(const Size &i) {
    return &_data[i];
  }

  Val *get(const Key &key) {
    return get_index(indexof(key));
  }

  Val read_index(const Size &i) {
    return _data[i];
  }

  Val read(const Key &key) {
    return read_index(indexof(key));
  }

  void write_index(const Size &i, const Val &val) {
    _data[i] = val;
  }

  void write(const Key &key, const Val &val) {
    write_index(indexof(key), val);
  }

  void insert_index(const Size &i) {
    extend(i);
    _present.set(i);
  }

  void insert(const Key &key) {
    insert_index(_forward->addkey(key));
  }

  void insert_index(const Size &i, const Val &val) {
    insert_index(i);
    write_index(i, val);
  }

  void insert(const Key &key, const Val &val) {
    insert_index(_forward->addkey(key), val);
  }

  void remove_index(const Size &i) {
    _present.reset(i);
  }

  void remove(const Key &key) {
    if (_forward->haskey(key)) {
      remove_index(indexof(key));
    }
  }

  IndexMap<Key, Val> *copy() {
    // TODO: if Val is a collection type, we need to deep copy.
    auto *copy = new IndexMap<Key, Val>(*this);

    return copy;
  }

  void clear() {
    _present.clear();
    _data.clear();
  }

  bool has_index(const Size &i) {
    return _present.test(i);
  }

  bool has(const Key &key) {
    if (_forward->haskey(key)) {
      return has_index(indexof(key));
    }
    return false;
  }

  Size size() {
    return this->size();
  }

  Vector<Key> *keys() {
    auto *keys = new Vector<Key>(this->size());
    Size i = 0;
    for (const auto &[key, _] : *this) {
      (*keys)[i++] = key;
    }
    return keys;
  }

  struct iterator {
    size_t _key;
    Val _val;
    Data::iterator _it;
    Data::iterator _ie;

    bool next() {
      if (this->_it == this->_ie) {
        return false;
      }
      ++this->_key;
      this->_val = *this->_it;
      this->_it = std::next(this->_it);
      return true;
    }
  };

  void begin(iterator *iter) {
    iter->_it = _data.begin();
    iter->_ie = _data.end();
  }
};

#endif // MEMOIR_BACKEND_STLUNORDEREDMAP_H
