// Simple hash table implemented in C.
#include <cstdint>
#include <cstdio>

#include <map>

#include <backend/stl_vector.h>

#define cname extern "C"
#define alwaysinline __attribute__((always_inline)) inline
#define used __attribute__((used))

extern "C" {

#define INSTANTIATE_stl_multimap(K, C_KEY, V, C_VALUE)                         \
  typedef std::multimap<C_KEY, C_VALUE> K##_##V##_stl_multimap_t;              \
  typedef K##_##V##_stl_multimap_t *K##_##V##_stl_multimap_p;                  \
                                                                               \
  typedef struct K##_##V##_stl_multimap_iter {                                 \
    C_KEY _key;                                                                \
    C_VALUE _val;                                                              \
    K##_##V##_stl_multimap_t::iterator _it;                                    \
    K##_##V##_stl_multimap_t::iterator _ie;                                    \
  } K##_##V##_stl_multimap_iter_t;                                             \
  typedef K##_##V##_stl_multimap_iter_t *K##_##V##_stl_multimap_iter_p;        \
                                                                               \
  cname alwaysinline used                                                      \
      K##_##V##_stl_multimap_p K##_##V##_stl_multimap__allocate(void) {        \
    K##_##V##_stl_multimap_p table = new K##_##V##_stl_multimap_t();           \
    return table;                                                              \
  }                                                                            \
                                                                               \
  cname alwaysinline used void K##_##V##_stl_multimap__free(                   \
      K##_##V##_stl_multimap_p table) {                                        \
    delete table;                                                              \
  }                                                                            \
                                                                               \
  cname alwaysinline used bool K##_##V##_stl_multimap__has(                    \
      K##_##V##_stl_multimap_p table,                                          \
      C_KEY key) {                                                             \
    return table->count(key) != 0;                                             \
  }                                                                            \
                                                                               \
  cname alwaysinline used C_VALUE *K##_##V##_stl_multimap__get(                \
      K##_##V##_stl_multimap_p table,                                          \
      C_KEY key,                                                               \
      size_t index) {                                                          \
    return (C_VALUE *)(&std::next(table->lower_bound(key), index)->second);    \
  }                                                                            \
                                                                               \
  cname alwaysinline used C_VALUE K##_##V##_stl_multimap__read(                \
      K##_##V##_stl_multimap_p table,                                          \
      C_KEY key,                                                               \
      size_t index) {                                                          \
    return std::next(table->lower_bound(key), index)->second;                  \
  }                                                                            \
                                                                               \
  cname alwaysinline used                                                      \
      K##_##V##_stl_multimap_p K##_##V##_stl_multimap__write(                  \
          K##_##V##_stl_multimap_p table,                                      \
          C_KEY key,                                                           \
          size_t index,                                                        \
          C_VALUE value) {                                                     \
    std::next(table->lower_bound(key), index)->second = value;                 \
    return table;                                                              \
  }                                                                            \
                                                                               \
  template <typename Key, typename Val>                                        \
  void K##_##V##_stl_multimap_insert_default(K##_##V##_stl_multimap_p table,   \
                                             Key key,                          \
                                             size_t index) {                   \
    if constexpr (std::is_pointer_v<Val>) {                                    \
      table->insert(std::next(table->lower_bound(key), index),                 \
                    std::make_pair(key, (Val)(nullptr)));                      \
    } else {                                                                   \
      table->insert(std::next(table->lower_bound(key), index),                 \
                    std::make_pair(key, Val()));                               \
    }                                                                          \
  }                                                                            \
                                                                               \
  cname alwaysinline used                                                      \
      K##_##V##_stl_multimap_p K##_##V##_stl_multimap__insert(                 \
          K##_##V##_stl_multimap_p table,                                      \
          C_KEY key,                                                           \
          size_t index) {                                                      \
    K##_##V##_stl_multimap_insert_default<C_KEY, C_VALUE>(table, key, index);  \
    return table;                                                              \
  }                                                                            \
                                                                               \
  cname alwaysinline used                                                      \
      K##_##V##_stl_multimap_p K##_##V##_stl_multimap__insert__1(              \
          K##_##V##_stl_multimap_p table,                                      \
          C_KEY key) {                                                         \
    return table;                                                              \
  }                                                                            \
                                                                               \
  cname alwaysinline used                                                      \
      K##_##V##_stl_multimap_p K##_##V##_stl_multimap__insert_value(           \
          K##_##V##_stl_multimap_p table,                                      \
          C_KEY key,                                                           \
          size_t index,                                                        \
          C_VALUE val) {                                                       \
    table->insert(std::next(table->lower_bound(key), index), { key, val });    \
    return table;                                                              \
  }                                                                            \
                                                                               \
  cname alwaysinline used                                                      \
      K##_##V##_stl_multimap_p K##_##V##_stl_multimap__remove(                 \
          K##_##V##_stl_multimap_p table,                                      \
          C_KEY key,                                                           \
          size_t index) {                                                      \
    table->erase(std::next(table->lower_bound(key), index));                   \
    return table;                                                              \
  }                                                                            \
                                                                               \
  cname alwaysinline used                                                      \
      K##_##V##_stl_multimap_p K##_##V##_stl_multimap__remove__1(              \
          K##_##V##_stl_multimap_p table,                                      \
          C_KEY key) {                                                         \
    table->erase(table->lower_bound(key), table->upper_bound(key));            \
    return table;                                                              \
  }                                                                            \
                                                                               \
  cname alwaysinline used                                                      \
      K##_##V##_stl_multimap_p K##_##V##_stl_multimap__clear(                  \
          K##_##V##_stl_multimap_p table) {                                    \
    table->clear();                                                            \
    return table;                                                              \
  }                                                                            \
                                                                               \
  cname alwaysinline used size_t K##_##V##_stl_multimap__size(                 \
      K##_##V##_stl_multimap_p table,                                          \
      C_KEY key) {                                                             \
    return table->count(key);                                                  \
  }                                                                            \
                                                                               \
  cname alwaysinline used size_t K##_##V##_stl_multimap__size__1(              \
      K##_##V##_stl_multimap_p table) {                                        \
    return table->size();                                                      \
  }                                                                            \
                                                                               \
  cname alwaysinline used K##_stl_vector_p K##_##V##_stl_multimap__keys(       \
      K##_##V##_stl_multimap_p table) {                                        \
    auto *keys = K##_stl_vector__allocate(table->size());                      \
    size_t i = 0;                                                              \
    for (const auto &[key, _] : *table) {                                      \
      (*keys)[i++] = key;                                                      \
    }                                                                          \
    return keys;                                                               \
  }                                                                            \
                                                                               \
  cname alwaysinline used void K##_##V##_stl_multimap__begin__1(               \
      K##_##V##_stl_multimap_iter_p iter,                                      \
      K##_##V##_stl_multimap_p table) {                                        \
    iter->_it = table->begin();                                                \
    iter->_ie = table->end();                                                  \
  }                                                                            \
  cname alwaysinline used bool K##_##V##_stl_multimap__next__1(                \
      K##_##V##_stl_multimap_iter_p iter) {                                    \
    if (iter->_it == iter->_ie) {                                              \
      return false;                                                            \
    }                                                                          \
    auto [_key, _val] = *iter->_it;                                            \
    iter->_key = _key;                                                         \
    iter->_val = _val;                                                         \
    ++iter->_it;                                                               \
    return true;                                                               \
  }                                                                            \
  cname alwaysinline used void K##_##V##_stl_multimap__begin(                  \
      K##_##V##_stl_multimap_iter_p iter,                                      \
      K##_##V##_stl_multimap_p table,                                          \
      C_KEY key) {                                                             \
    iter->_it = table->lower_bound(key);                                       \
    iter->_ie = table->upper_bound(key);                                       \
  }                                                                            \
  cname alwaysinline used bool K##_##V##_stl_multimap__next(                   \
      K##_##V##_stl_multimap_iter_p iter) {                                    \
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
