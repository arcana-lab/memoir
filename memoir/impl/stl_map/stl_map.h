// Simple hash table implemented in C.
#include <cstdint>
#include <cstdio>

#include <map>

#include <backend/stl_vector.h>

#define CNAME extern "C"
#define ALWAYS_INLINE __attribute__((always_inline)) inline
#define USED __attribute__((USED))

extern "C" {

#define INSTANTIATE_stl_map(K, C_KEY, V, C_VALUE)                              \
  typedef std::map<C_KEY, C_VALUE> K##_##V##_stl_map_t;                        \
  typedef K##_##V##_stl_map_t *K##_##V##_stl_map_p;                            \
                                                                               \
  typedef struct K##_##V##_stl_map_iter {                                      \
    C_KEY _key;                                                                \
    C_VALUE _val;                                                              \
    K##_##V##_stl_map_t::iterator _it;                                         \
    K##_##V##_stl_map_t::iterator _ie;                                         \
  } K##_##V##_stl_map_iter_t;                                                  \
  typedef K##_##V##_stl_map_iter_t *K##_##V##_stl_map_iter_p;                  \
                                                                               \
  typedef struct K##_##V##_stl_map_riter {                                     \
    C_KEY _key;                                                                \
    C_VALUE _val;                                                              \
    K##_##V##_stl_map_t::reverse_iterator _it;                                 \
    K##_##V##_stl_map_t::reverse_iterator _ie;                                 \
  } K##_##V##_stl_map_riter_t;                                                 \
  typedef K##_##V##_stl_map_riter_t *K##_##V##_stl_map_riter_p;                \
                                                                               \
  CNAME ALWAYS_INLINE USED K##_##V##_stl_map_p K##_##V##_stl_map__allocate(    \
      void) {                                                                  \
    K##_##V##_stl_map_p table = new K##_##V##_stl_map_t();                     \
    return table;                                                              \
  }                                                                            \
                                                                               \
  CNAME ALWAYS_INLINE USED void K##_##V##_stl_map__free(                       \
      K##_##V##_stl_map_p table) {                                             \
    delete table;                                                              \
  }                                                                            \
                                                                               \
  CNAME ALWAYS_INLINE USED bool K##_##V##_stl_map__has(                        \
      K##_##V##_stl_map_p table,                                               \
      C_KEY key) {                                                             \
    return table->count(key) != 0;                                             \
  }                                                                            \
                                                                               \
  CNAME ALWAYS_INLINE USED C_VALUE *K##_##V##_stl_map__get(                    \
      K##_##V##_stl_map_p table,                                               \
      C_KEY key) {                                                             \
    return (C_VALUE *)(&((*table)[key]));                                      \
  }                                                                            \
                                                                               \
  CNAME ALWAYS_INLINE USED C_VALUE K##_##V##_stl_map__read(                    \
      K##_##V##_stl_map_p table,                                               \
      C_KEY key) {                                                             \
    return (*table)[key];                                                      \
  }                                                                            \
                                                                               \
  CNAME ALWAYS_INLINE USED K##_##V##_stl_map_p K##_##V##_stl_map__write(       \
      K##_##V##_stl_map_p table,                                               \
      C_KEY key,                                                               \
      C_VALUE value) {                                                         \
    (*table)[key] = value;                                                     \
    return table;                                                              \
  }                                                                            \
                                                                               \
  CNAME ALWAYS_INLINE USED K##_##V##_stl_map_p K##_##V##_stl_map__insert(      \
      K##_##V##_stl_map_p table,                                               \
      C_KEY key) {                                                             \
    (*table)[key];                                                             \
    return table;                                                              \
  }                                                                            \
                                                                               \
  CNAME ALWAYS_INLINE USED K##_##V##_stl_map_p K##_##V##_stl_map__remove(      \
      K##_##V##_stl_map_p table,                                               \
      C_KEY key) {                                                             \
    table->erase(key);                                                         \
    return table;                                                              \
  }                                                                            \
                                                                               \
  CNAME ALWAYS_INLINE USED K##_##V##_stl_map_p K##_##V##_stl_map__clear(       \
      K##_##V##_stl_map_p table) {                                             \
    table->clear();                                                            \
    return table;                                                              \
  }                                                                            \
                                                                               \
  CNAME ALWAYS_INLINE USED size_t K##_##V##_stl_map__size(                     \
      K##_##V##_stl_map_p table) {                                             \
    return table->size();                                                      \
  }                                                                            \
  CNAME ALWAYS_INLINE USED K##_stl_vector_p K##_##V##_stl_map__keys(           \
      K##_##V##_stl_map_p table) {                                             \
    auto *keys = K##_stl_vector__allocate(table->size());                      \
    size_t i = 0;                                                              \
    for (const auto &[key, _] : *table) {                                      \
      (*keys)[i++] = key;                                                      \
    }                                                                          \
    return keys;                                                               \
  }                                                                            \
  CNAME ALWAYS_INLINE USED void K##_##V##_stl_map__begin(                      \
      K##_##V##_stl_map_iter_p iter,                                           \
      K##_##V##_stl_map_p vec) {                                               \
    iter->_it = vec->begin();                                                  \
    iter->_ie = vec->end();                                                    \
  }                                                                            \
  CNAME ALWAYS_INLINE USED bool K##_##V##_stl_map__next(                       \
      K##_##V##_stl_map_iter_p iter) {                                         \
    if (iter->_it == iter->_ie) {                                              \
      return false;                                                            \
    }                                                                          \
    auto [_key, _val] = *iter->_it;                                            \
    iter->_key = _key;                                                         \
    iter->_val = _val;                                                         \
    ++iter->_it;                                                               \
    return true;                                                               \
  }                                                                            \
  CNAME ALWAYS_INLINE USED void K##_##V##_stl_map__rbegin(                     \
      K##_##V##_stl_map_riter_p iter,                                          \
      K##_##V##_stl_map_p vec) {                                               \
    iter->_it = vec->rbegin();                                                 \
    iter->_ie = vec->rend();                                                   \
  }                                                                            \
  CNAME ALWAYS_INLINE USED bool K##_##V##_stl_map__rnext(                      \
      K##_##V##_stl_map_iter_p iter) {                                         \
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
