// Simple hash table implemented in C.
#include <cstdint>
#include <cstdio>

#include <boost/dynamic_bitset.hpp>

#include <vector>

#define cname extern "C"
#define alwaysinline __attribute__((always_inline)) inline
#define used __attribute__((used))

#ifndef FOLIO_BITMAP
#  define FOLIO_BITMAP

template <typename KeyTy, typename ValTy>
class BitMap {
public:
  BitMap(std::size_t size) : _bits(size), _vals(size) {}

  BitMap() : BitMap(1) {}

  void insert(KeyTy value) {
    if (value >= _bits.size()) {
      _bits.resize(value + 1);
    }
    if (value >= _vals.size()) {
      _vals.resize(value + 1);
    }

    _bits.set(value);
  }

  void remove(KeyTy value) {
    if (value >= _bits.size()) {
      _bits.resize(value + 1);
    }
    _bits.reset(value);
  }

  bool has(KeyTy key) const {
    if (key >= _bits.size()) {
      return false;
    }
    return _bits.test(key);
  }

  ValTy read(KeyTy key) {
    return _vals[key];
  }

  template <typename T = ValTy,
            typename std::enable_if_t<!std::is_same_v<T, bool>, bool> = 0>
  ValTy *get(KeyTy key) {
    return &_vals[key];
  }

  template <typename T = ValTy,
            typename std::enable_if_t<std::is_same_v<T, bool>, bool> = 0>
  ValTy *get(KeyTy key) {
    return nullptr;
  }

  void write(KeyTy key, ValTy val) {
    _vals[key] = val;
  }

  struct iterator {

    iterator &operator=(const iterator &x) {
      _cur = x._cur;
      _bits = x._bits;
      _values = x._values;

      return *this;
    }

    iterator(std::vector<ValTy> *values, boost::dynamic_bitset<> *bits)
      : _values(values),
        _bits(bits),
        _cur(bits->find_first()) {}

    std::vector<ValTy> *_values;
    boost::dynamic_bitset<> *_bits;
    size_t _cur;

    iterator &operator++() {
      _cur = _bits->find_next(_cur);

      return *this;
    }

    std::pair<KeyTy, ValTy> operator*() {
      return { _cur, (*_values)[_cur] };
    }

    bool done() {
      return _cur == boost::dynamic_bitset<>::npos;
    }
  };

  iterator begin() {
    return iterator(&this->_vals, &this->_bits);
  }

  iterator end() {
    return iterator(&this->_vals, &this->_bits);
  }

private:
  boost::dynamic_bitset<> _bits;
  std::vector<ValTy> _vals;
};

#endif // FOLIO_BITMAP

extern "C" {

#define INSTANTIATE_bitmap(K, C_KEY, V, C_VALUE)                               \
  typedef BitMap<C_KEY, C_VALUE> K##_##V##_bitmap_t;                           \
  typedef K##_##V##_bitmap_t *K##_##V##_bitmap_p;                              \
                                                                               \
  typedef struct K##_##V##_bitmap_iter {                                       \
    C_KEY _key;                                                                \
    C_VALUE _val;                                                              \
    K##_##V##_bitmap_t::iterator _it;                                          \
  } K##_##V##_bitmap_iter_t;                                                   \
  typedef K##_##V##_bitmap_iter_t *K##_##V##_bitmap_iter_p;                    \
                                                                               \
  cname alwaysinline used K##_##V##_bitmap_p K##_##V##_bitmap__allocate(       \
      void) {                                                                  \
    K##_##V##_bitmap_p map = new K##_##V##_bitmap_t();                         \
    return map;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used void K##_##V##_bitmap__free(                         \
      K##_##V##_bitmap_p map) {                                                \
    delete map;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used bool K##_##V##_bitmap__has(K##_##V##_bitmap_p map,   \
                                                     C_KEY key) {              \
    return map->has(key);                                                      \
  }                                                                            \
                                                                               \
  cname alwaysinline used C_VALUE K##_##V##_bitmap__read(                      \
      K##_##V##_bitmap_p map,                                                  \
      C_KEY key) {                                                             \
    return map->read(key);                                                     \
  }                                                                            \
                                                                               \
  cname alwaysinline used K##_##V##_bitmap_p K##_##V##_bitmap__write(          \
      K##_##V##_bitmap_p map,                                                  \
      C_KEY key,                                                               \
      C_VALUE value) {                                                         \
    map->write(key, value);                                                    \
    return map;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used C_VALUE *K##_##V##_bitmap__get(                      \
      K##_##V##_bitmap_p map,                                                  \
      C_KEY key) {                                                             \
    return map->get(key);                                                      \
  }                                                                            \
                                                                               \
  cname alwaysinline used K##_##V##_bitmap_p K##_##V##_bitmap__insert(         \
      K##_##V##_bitmap_p map,                                                  \
      C_KEY key) {                                                             \
    map->insert(key);                                                          \
    return map;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used K##_##V##_bitmap_p K##_##V##_bitmap__remove(         \
      K##_##V##_bitmap_p map,                                                  \
      C_KEY key) {                                                             \
    map->remove(key);                                                          \
    return map;                                                                \
  }                                                                            \
                                                                               \
  /*cname alwaysinline used size_t K##_##V##_bitmap__size(             */      \
  /*K##_##V##_bitmap_p map) {                                          */      \
  /*  return map->size();                                                */    \
  /*} */                                                                       \
  cname alwaysinline used void K##_##V##_bitmap__begin(                        \
      K##_##V##_bitmap_iter_p iter,                                            \
      K##_##V##_bitmap_p map) {                                                \
    iter->_it = map->begin();                                                  \
  }                                                                            \
  cname alwaysinline used bool K##_##V##_bitmap__next(                         \
      K##_##V##_bitmap_iter_p iter) {                                          \
    if (iter->_it.done()) {                                                    \
      return false;                                                            \
    }                                                                          \
    auto [_key, _val] = *iter->_it;                                            \
    iter->_key = _key;                                                         \
    iter->_val = _val;                                                         \
    ++iter->_it;                                                               \
    return true;                                                               \
  }

} // extern "C"
