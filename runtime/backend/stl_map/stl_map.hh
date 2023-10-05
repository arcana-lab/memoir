// Simple hash table implemented in C.
#include <cstdint>
#include <cstdio>

#include <map>

#define cname extern "C"
#define alwaysinline __attribute__((always_inline)) inline
#define used __attribute__((used))

extern "C" {

#define INSTANTIATE_STL_MAP(K, C_KEY, V, C_VALUE)                              \
  typedef std::map<C_KEY, C_VALUE> K##_##V##_stl_map_t;                        \
  typedef K##_##V##_stl_map_t *K##_##V##_stl_map_p;                            \
                                                                               \
  cname alwaysinline used K##_##V##_stl_map_p K##_##V##_stl_map__allocate(     \
      void) {                                                                  \
    K##_##V##_stl_map_p table = new K##_##V##_stl_map_t();                     \
    return table;                                                              \
  }                                                                            \
                                                                               \
  cname alwaysinline used void K##_##V##_stl_map__free(                        \
      K##_##V##_stl_map_p table) {                                             \
    delete table;                                                              \
  }                                                                            \
                                                                               \
  cname alwaysinline used bool K##_##V##_stl_map__has(                         \
      K##_##V##_stl_map_p table,                                               \
      C_KEY key) {                                                             \
    return table->count(key) != 0;                                             \
  }                                                                            \
                                                                               \
  /* Get key-value pair */                                                     \
  cname alwaysinline used C_VALUE *K##_##V##_stl_map__get(                     \
      K##_##V##_stl_map_p table,                                               \
      C_KEY key) {                                                             \
    return (C_VALUE *)(&((*table)[key]));                                      \
  }                                                                            \
                                                                               \
  cname alwaysinline used K##_##V##_stl_map_p K##_##V##_stl_map__remove(       \
      K##_##V##_stl_map_p table,                                               \
      C_KEY key) {                                                             \
    table->erase(key);                                                         \
    return table;                                                              \
  }                                                                            \
                                                                               \
  cname alwaysinline used size_t K##_##V##_stl_map__size(                      \
      K##_##V##_stl_map_p table) {                                             \
    return table->size();                                                      \
  }

} // extern "C"
