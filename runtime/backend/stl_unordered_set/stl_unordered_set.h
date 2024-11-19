// Simple hash table implemented in C.
#include <cstdint>
#include <cstdio>

#include <unordered_set>

#include <backend/stl_vector.h>

#define cname extern "C"
#define alwaysinline __attribute__((always_inline)) inline
#define used __attribute__((used))

extern "C" {

#define INSTANTIATE_stl_unordered_set(K, C_KEY, V, C_VALUE)                    \
  typedef std::unordered_set<C_KEY> K##_##V##_stl_unordered_set_t;             \
  typedef K##_##V##_stl_unordered_set_t *K##_##V##_stl_unordered_set_p;        \
                                                                               \
  typedef struct K##_##V##_stl_unordered_set_iter {                            \
    C_KEY _key;                                                                \
    K##_##V##_stl_unordered_set_t::iterator _it;                               \
    K##_##V##_stl_unordered_set_t::iterator _ie;                               \
  } K##_##V##_stl_unordered_set_iter_t;                                        \
  typedef K##_##V##_stl_unordered_set_iter_t                                   \
      *K##_##V##_stl_unordered_set_iter_p;                                     \
                                                                               \
  cname alwaysinline used                                                      \
      K##_##V##_stl_unordered_set_p K##_##V##_stl_unordered_set__allocate(     \
          void) {                                                              \
    K##_##V##_stl_unordered_set_p table = new K##_##V##_stl_unordered_set_t(); \
    return table;                                                              \
  }                                                                            \
                                                                               \
  cname alwaysinline used void K##_##V##_stl_unordered_set__free(              \
      K##_##V##_stl_unordered_set_p table) {                                   \
    delete table;                                                              \
  }                                                                            \
                                                                               \
  cname alwaysinline used bool K##_##V##_stl_unordered_set__has(               \
      K##_##V##_stl_unordered_set_p table,                                     \
      C_KEY key) {                                                             \
    return table->count(key) != 0;                                             \
  }                                                                            \
                                                                               \
  cname alwaysinline used                                                      \
      K##_##V##_stl_unordered_set_p K##_##V##_stl_unordered_set__insert(       \
          K##_##V##_stl_unordered_set_p table,                                 \
          C_KEY key) {                                                         \
    (*table).insert(key);                                                      \
    return table;                                                              \
  }                                                                            \
                                                                               \
  cname alwaysinline used                                                      \
      K##_##V##_stl_unordered_set_p K##_##V##_stl_unordered_set__remove(       \
          K##_##V##_stl_unordered_set_p table,                                 \
          C_KEY key) {                                                         \
    table->erase(key);                                                         \
    return table;                                                              \
  }                                                                            \
                                                                               \
  cname alwaysinline used                                                      \
      K##_##V##_stl_unordered_set_p K##_##V##_stl_unordered_set__clear(        \
          K##_##V##_stl_unordered_set_p table) {                               \
    table->clear();                                                            \
    return table;                                                              \
  }                                                                            \
                                                                               \
  cname alwaysinline used size_t K##_##V##_stl_unordered_set__size(            \
      K##_##V##_stl_unordered_set_p table) {                                   \
    return table->size();                                                      \
  }                                                                            \
  cname alwaysinline used K##_stl_vector_p K##_##V##_stl_unordered_set__keys(  \
      K##_##V##_stl_unordered_set_p table) {                                   \
    auto *keys = K##_stl_vector__allocate(table->size());                      \
    size_t i = 0;                                                              \
    for (const auto &key : *table) {                                           \
      (*keys)[i++] = key;                                                      \
    }                                                                          \
    return keys;                                                               \
  }                                                                            \
  cname alwaysinline used void K##_##V##_stl_unordered_set__begin(             \
      K##_##V##_stl_unordered_set_iter_p iter,                                 \
      K##_##V##_stl_unordered_set_p vec) {                                     \
    iter->_it = vec->begin();                                                  \
    iter->_ie = vec->end();                                                    \
  }                                                                            \
  cname alwaysinline used bool K##_##V##_stl_unordered_set__next(              \
      K##_##V##_stl_unordered_set_iter_p iter) {                               \
    if (iter->_it == iter->_ie) {                                              \
      return false;                                                            \
    }                                                                          \
    iter->_key = *iter->_it;                                                   \
    ++iter->_it;                                                               \
    return true;                                                               \
  }

} // extern "C"
