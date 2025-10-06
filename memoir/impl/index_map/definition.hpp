#ifndef MEMOIR_BACKEND_INDEXMAP_H
#define MEMOIR_BACKEND_INDEXMAP_H

#include <cstdint>
#include <cstdio>
#include <functional>
#include <type_traits>

#include <boost/dynamic_bitset.hpp>

#include <memoir/impl/stl_vector/definition.hpp>

using Size = size_t;

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

  using Encoder = std::unordered_map<Key, Size>;
  Encoder *_encoder;

  using Decoder = std::vector<Key>;
  Decoder *_decoder;

  IndexMap() : _present(), _data(), _encoder(new Encoder()) {}
  IndexMap(Encoder *encoder) : _present(), _data(), _encoder(encoder) {}
  IndexMap(const IndexMap<Key, Val> &other)
    : _present(other._present),
      _data(other._data),
      _encoder(other._encoder) {}
  ~IndexMap() {
    // TODO: if the element is a collection pointer, delete it too.
  }

  // ====================================
  // === IndexMap specific operations ===
  Encoder *encoder() {
    return _encoder;
  }

  void encoder(Encoder *enc) {
    _encoder = enc;
  }

  Size indexof(const Key &key) {
    return _encoder->at(key);
  }

  Size addkey(const Key &key) {
    auto i = _encoder->size();
    _encoder->insert({ key, i });
    return i;
  }

  bool haskey(const Key &key) {
    return _encoder->count(key) > 0;
  }

  Decoder *decoder() {
    return _decoder;
  }

  void decoder(Decoder *dec) {
    _decoder = dec;
  }

  const Key &keyof(const Size &index) {
    return _decoder->keyof(index);
  }
  // ====================================

  Val *get_encoded(const Size &i) {
    return &_data[i];
  }

  Val *get(const Key &key) {
    return get_encoded(indexof(key));
  }

  Val read_encoded(const Size &i) {
    return _data[i];
  }

  Val read(const Key &key) {
    return read_encoded(indexof(key));
  }

  void write_encoded(const Size &i, const Val &val) {
    _data[i] = val;
  }

  void write(const Key &key, const Val &val) {
    write_encoded(indexof(key), val);
  }

  void insert_encoded(const Size &i) {
    extend(i);
    _present.set(i);
  }

  void insert(const Key &key) {
    insert_encoded(addkey(key));
  }

  void insert_encoded(const Size &i, const Val &val) {
    insert_encoded(i);
    write_encoded(i, val);
  }

  void insert(const Key &key, const Val &val) {
    insert_encoded(addkey(key), val);
  }

  void remove_encoded(const Size &i) {
    _present.reset(i);
  }

  void remove(const Key &key) {
    if (haskey(key)) {
      remove_encoded(indexof(key));
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

  bool has_encoded(const Size &i) {
    return _present.test(i);
  }

  bool has(const Key &key) {
    if (haskey(key)) {
      return has_encoded(indexof(key));
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

#endif // MEMOIR_BACKEND_INDEXMAP_H
