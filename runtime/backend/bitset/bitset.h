// Simple hash table implemented in C.

#include <cstdint>
#include <cstdio>

#include <bitset>
#include <vector>

#define cname extern "C"
#define alwaysinline __attribute__((always_inline)) inline
#define used __attribute__((used))

#ifndef FOLIO_BITSET
#  define FOLIO_BITSET

template <typename KeyTy>
class BitSet {
public:
  BitSet(std::size_t size) : _buf((size + 7) / 8) {}

  BitSet() : BitSet(1) {}

  void insert(KeyTy value) {
    if (value >= this->capacity()) {
      _buf.resize(value / 8 + 1);
    }

    _buf[value / 8] |= 1 << (value % 8);
  }

  void remove(KeyTy value) {
    if (value >= this->capacity()) {
      _buf.resize(value / 8 + 1);
    }
    _buf[value / 8] &= (0xFF ^ (1 << (value % 8)));
  }

  bool has(KeyTy key) const {
    if (key >= this->capacity()) {
      return false;
    }
    return _buf[key / 8] & (1 << (key % 8));
  }

  bool size() const {
    size_t n = 0;
    for (auto val : _buf) {
      n += std::bitset<8>(val).count();
    }
    return n;
  }

  struct iterator {

    iterator &operator=(const iterator &x) {
      _buf_it = x._buf_it;
      _buf_ib = x._buf_ib;
      _buf_ie = x._buf_ie;
      _curr_bit = x._curr_bit;

      return *this;
    }

    iterator(std::vector<uint8_t>::iterator it,
             std::vector<uint8_t>::iterator ie)
      : _buf_it(it),
        _buf_ib(it),
        _buf_ie(ie),
        _curr_bit(0) {}

    std::vector<uint8_t>::iterator _buf_it;
    std::vector<uint8_t>::iterator _buf_ib;
    std::vector<uint8_t>::iterator _buf_ie;
    uint8_t _curr_bit;

    iterator &operator++() {
      do {
        auto bits = *_buf_it;
        ++_curr_bit;
        for (; _curr_bit < 8; ++_curr_bit) {
          if (bits & (1 << _curr_bit)) {
            return *this;
          }
        }
        _curr_bit = 0;
      } while (++_buf_it != _buf_ie);

      return *this;
    }

    KeyTy operator*() {
      // Compute the key.
      auto key = std::distance(_buf_ib, _buf_it) * 8 + _curr_bit;
      return key;
    }

    bool done() {
      return (_buf_it == _buf_ie);
    }
  };

  iterator begin() {
    return iterator(_buf.begin(), _buf.end());
  }

  iterator end() {
    return iterator(_buf.end(), _buf.end());
  }

private:
  std::size_t capacity() const {
    return (_buf.size() * 8);
  }

  std::vector<uint8_t> _buf;
};

#endif

extern "C" {

#define INSTANTIATE_bitset(K, C_KEY, V, C_VALUE)                               \
  typedef BitSet<C_KEY> K##_##V##_bitset_t;                                    \
  typedef K##_##V##_bitset_t *K##_##V##_bitset_p;                              \
                                                                               \
  typedef struct K##_##V##_bitset_iter {                                       \
    C_KEY _key;                                                                \
    K##_##V##_bitset_t::iterator _it;                                          \
  } K##_##V##_bitset_iter_t;                                                   \
  typedef K##_##V##_bitset_iter_t *K##_##V##_bitset_iter_p;                    \
                                                                               \
  cname alwaysinline used K##_##V##_bitset_p K##_##V##_bitset__allocate(       \
      void) {                                                                  \
    K##_##V##_bitset_p set = new K##_##V##_bitset_t();                         \
    return set;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used void K##_##V##_bitset__free(                         \
      K##_##V##_bitset_p set) {                                                \
    delete set;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used bool K##_##V##_bitset__has(K##_##V##_bitset_p set,   \
                                                     C_KEY key) {              \
    return set->has(key);                                                      \
  }                                                                            \
                                                                               \
  cname alwaysinline used K##_##V##_bitset_p K##_##V##_bitset__insert(         \
      K##_##V##_bitset_p set,                                                  \
      C_KEY key) {                                                             \
    set->insert(key);                                                          \
    return set;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used K##_##V##_bitset_p K##_##V##_bitset__remove(         \
      K##_##V##_bitset_p set,                                                  \
      C_KEY key) {                                                             \
    set->remove(key);                                                          \
    return set;                                                                \
  }                                                                            \
  cname alwaysinline used size_t K##_##V##_bitset__size(                       \
      K##_##V##_bitset_p set) {                                                \
    return set->size();                                                        \
  }                                                                            \
  cname alwaysinline used void K##_##V##_bitset__begin(                        \
      K##_##V##_bitset_iter_p iter,                                            \
      K##_##V##_bitset_p set) {                                                \
    iter->_it = set->begin();                                                  \
  }                                                                            \
  cname alwaysinline used bool K##_##V##_bitset__next(                         \
      K##_##V##_bitset_iter_p iter) {                                          \
    if (iter->_it.done()) {                                                    \
      return false;                                                            \
    }                                                                          \
    iter->_key = *iter->_it;                                                   \
    ++iter->_it;                                                               \
    return true;                                                               \
  }

} // extern "C"
