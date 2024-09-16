// Simple hash table implemented in C.
#include <cstdint>
#include <cstdio>

#include <unordered_map>

#include <backend/stl_vector.h>

#define cname extern "C"
#define alwaysinline __attribute__((always_inline)) inline
#define used __attribute__((used))

extern "C" {

#define INSTANTIATE_stl_unordered_map(K, C_KEY, V, C_VALUE)                    \
  typedef std::unordered_map<C_KEY, C_VALUE> K##_##V##_stl_unordered_map_t;    \
  typedef K##_##V##_stl_unordered_map_t *K##_##V##_stl_unordered_map_p;        \
                                                                               \
  typedef struct K##_##V##_stl_unordered_map_iter {                            \
    C_KEY _key;                                                                \
    C_VALUE _val;                                                              \
    K##_##V##_stl_unordered_map_t::iterator _it;                               \
    K##_##V##_stl_unordered_map_t::iterator _ie;                               \
  } K##_##V##_stl_unordered_map_iter_t;                                        \
  typedef K##_##V##_stl_unordered_map_iter_t                                   \
      *K##_##V##_stl_unordered_map_iter_p;                                     \
                                                                               \
  typedef struct K##_##V##_stl_unordered_map_riter {                           \
    C_KEY _key;                                                                \
    C_VALUE _val;                                                              \
    K##_##V##_stl_unordered_map_t::reverse_iterator _it;                       \
    K##_##V##_stl_unordered_map_t::reverse_iterator _ie;                       \
  } K##_##V##_stl_unordered_map_riter_t;                                       \
  typedef K##_##V##_stl_unordered_map_riter_t                                  \
      *K##_##V##_stl_unordered_map_riter_p;                                    \
                                                                               \
  cname alwaysinline used                                                      \
      K##_##V##_stl_unordered_map_p K##_##V##_stl_unordered_map__allocate(     \
          void) {                                                              \
    K##_##V##_stl_unordered_map_p table = new K##_##V##_stl_unordered_map_t(); \
    return table;                                                              \
  }                                                                            \
                                                                               \
  cname alwaysinline used void K##_##V##_stl_unordered_map__free(              \
      K##_##V##_stl_unordered_map_p table) {                                   \
    delete table;                                                              \
  }                                                                            \
                                                                               \
  cname alwaysinline used bool K##_##V##_stl_unordered_map__has(               \
      K##_##V##_stl_unordered_map_p table,                                     \
      C_KEY key) {                                                             \
    return table->count(key) != 0;                                             \
  }                                                                            \
                                                                               \
  cname alwaysinline used C_VALUE *K##_##V##_stl_unordered_map__get(           \
      K##_##V##_stl_unordered_map_p table,                                     \
      C_KEY key) {                                                             \
    return (C_VALUE *)(&((*table)[key]));                                      \
  }                                                                            \
                                                                               \
  cname alwaysinline used C_VALUE K##_##V##_stl_unordered_map__read(           \
      K##_##V##_stl_unordered_map_p table,                                     \
      C_KEY key) {                                                             \
    return (*table)[key];                                                      \
  }                                                                            \
                                                                               \
  cname alwaysinline used                                                      \
      K##_##V##_stl_unordered_map_p K##_##V##_stl_unordered_map__write(        \
          K##_##V##_stl_unordered_map_p table,                                 \
          C_KEY key,                                                           \
          C_VALUE value) {                                                     \
    (*table)[key] = value;                                                     \
    return table;                                                              \
  }                                                                            \
                                                                               \
  cname alwaysinline used                                                      \
      K##_##V##_stl_unordered_map_p K##_##V##_stl_unordered_map__insert(       \
          K##_##V##_stl_unordered_map_p table,                                 \
          C_KEY key) {                                                         \
    (*table)[key];                                                             \
    return table;                                                              \
  }                                                                            \
                                                                               \
  cname alwaysinline used                                                      \
      K##_##V##_stl_unordered_map_p K##_##V##_stl_unordered_map__remove(       \
          K##_##V##_stl_unordered_map_p table,                                 \
          C_KEY key) {                                                         \
    table->erase(key);                                                         \
    return table;                                                              \
  }                                                                            \
                                                                               \
  cname alwaysinline used size_t K##_##V##_stl_unordered_map__size(            \
      K##_##V##_stl_unordered_map_p table) {                                   \
    return table->size();                                                      \
  }                                                                            \
  cname alwaysinline used K##_stl_vector_p K##_##V##_stl_unordered_map__keys(  \
      K##_##V##_stl_unordered_map_p table) {                                   \
    auto *keys = K##_stl_vector__allocate(table->size());                      \
    size_t i = 0;                                                              \
    for (const auto &[key, _] : *table) {                                      \
      (*keys)[i++] = key;                                                      \
    }                                                                          \
    return keys;                                                               \
  }                                                                            \
  cname alwaysinline used void K##_##V##_stl_unordered_map__begin(             \
      K##_##V##_stl_unordered_map_iter_p iter,                                 \
      K##_##V##_stl_unordered_map_p vec) {                                     \
    iter->_it = vec->begin();                                                  \
    iter->_ie = vec->end();                                                    \
  }                                                                            \
  cname alwaysinline used bool K##_##V##_stl_unordered_map__next(              \
      K##_##V##_stl_unordered_map_iter_p iter) {                               \
    if (iter->_it == iter->_ie) {                                              \
      return false;                                                            \
    }                                                                          \
    auto [_key, _val] = *iter->_it;                                            \
    iter->_key = _key;                                                         \
    iter->_val = _val;                                                         \
    ++iter->_it;                                                               \
    return true;                                                               \
  }                                                                            \
  cname alwaysinline used void K##_##V##_stl_unordered_map__rbegin(            \
      K##_##V##_stl_unordered_map_riter_p iter,                                \
      K##_##V##_stl_unordered_map_p vec) {                                     \
    iter->_it = vec->rbegin();                                                 \
    iter->_ie = vec->rend();                                                   \
  }                                                                            \
  cname alwaysinline used bool K##_##V##_stl_unordered_map__rnext(             \
      K##_##V##_stl_unordered_map_iter_p iter) {                               \
    if (iter->_it == iter->_ie) {                                              \
      return false;                                                            \
    }                                                                          \
    auto [_key, _val] = *iter->_it;                                            \
    iter->_key = _key;                                                         \
    iter->_val = _val;                                                         \
    ++iter->_it;                                                               \
    return true;                                                               \
  }

} // extern "C"
