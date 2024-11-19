// Simple hash table implemented in C.
#include <cstdint>
#include <cstdio>

#include <boost/dynamic_bitset.hpp>
#include <vector>

#define cname extern "C"
#define alwaysinline __attribute__((always_inline)) inline
#define used __attribute__((used))

extern "C" {

#define INSTANTIATE_boost_dynamic_bitset(K, C_KEY, V, C_VALUE)                 \
  typedef boost::dynamic_bitset<> K##_##V##_boost_dynamic_bitset_t;            \
  typedef K##_##V##_boost_dynamic_bitset_t *K##_##V##_boost_dynamic_bitset_p;  \
                                                                               \
  typedef struct K##_##V##_boost_dynamic_bitset_iter {                         \
    C_KEY _key;                                                                \
    size_t _cur;                                                               \
    K##_##V##_boost_dynamic_bitset_p _set;                                     \
  } K##_##V##_boost_dynamic_bitset_iter_t;                                     \
  typedef K##_##V##_boost_dynamic_bitset_iter_t                                \
      *K##_##V##_boost_dynamic_bitset_iter_p;                                  \
                                                                               \
  cname alwaysinline used K##_##V##_boost_dynamic_bitset_p                     \
      K##_##V##_boost_dynamic_bitset__allocate(void) {                         \
    K##_##V##_boost_dynamic_bitset_p set =                                     \
        new K##_##V##_boost_dynamic_bitset_t();                                \
    return set;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used void K##_##V##_boost_dynamic_bitset__free(           \
      K##_##V##_boost_dynamic_bitset_p set) {                                  \
    delete set;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used bool K##_##V##_boost_dynamic_bitset__has(            \
      K##_##V##_boost_dynamic_bitset_p set,                                    \
      C_KEY key) {                                                             \
    if (key >= set->size()) {                                                  \
      return false;                                                            \
    }                                                                          \
    return set->test(key);                                                     \
  }                                                                            \
                                                                               \
  cname alwaysinline used                                                      \
      K##_##V##_boost_dynamic_bitset_p K##_##V##_boost_dynamic_bitset__insert( \
          K##_##V##_boost_dynamic_bitset_p set,                                \
          C_KEY key) {                                                         \
    if (key >= set->size()) {                                                  \
      set->resize(key + 1);                                                    \
    }                                                                          \
    set->set(key);                                                             \
    return set;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used                                                      \
      K##_##V##_boost_dynamic_bitset_p K##_##V##_boost_dynamic_bitset__remove( \
          K##_##V##_boost_dynamic_bitset_p set,                                \
          C_KEY key) {                                                         \
    if (key >= set->size()) {                                                  \
      set->resize(key + 1);                                                    \
    }                                                                          \
    set->reset(key);                                                           \
    return set;                                                                \
  }                                                                            \
  cname alwaysinline used size_t K##_##V##_boost_dynamic_bitset__size(         \
      K##_##V##_boost_dynamic_bitset_p set) {                                  \
    return set->count();                                                       \
  }                                                                            \
  cname alwaysinline used                                                      \
      K##_##V##_boost_dynamic_bitset_p K##_##V##_boost_dynamic_bitset__clear(  \
          K##_##V##_boost_dynamic_bitset_p set) {                              \
    set->clear();                                                              \
    return set;                                                                \
  }                                                                            \
  cname alwaysinline used void K##_##V##_boost_dynamic_bitset__begin(          \
      K##_##V##_boost_dynamic_bitset_iter_p iter,                              \
      K##_##V##_boost_dynamic_bitset_p set) {                                  \
    iter->_cur = set->find_first();                                            \
    iter->_set = set;                                                          \
  }                                                                            \
  cname alwaysinline used bool K##_##V##_boost_dynamic_bitset__next(           \
      K##_##V##_boost_dynamic_bitset_iter_p iter) {                            \
    if (iter->_cur == K##_##V##_boost_dynamic_bitset_t::npos) {                \
      return false;                                                            \
    }                                                                          \
    iter->_key = iter->_cur;                                                   \
    iter->_cur = iter->_set->find_next(iter->_cur);                            \
    return true;                                                               \
  }

} // extern "C"
