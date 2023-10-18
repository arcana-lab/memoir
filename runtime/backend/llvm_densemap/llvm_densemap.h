// Simple hash table implemented in C.
#include "llvm/ADT/DenseMap.h"

#include <cstdint>
#include <cstdio>

#include <vector>

#define cname extern "C"
#define alwaysinline __attribute__((always_inline)) inline
#define used __attribute__((used))

#define SMALL_SIZE 256

extern "C" {

#define INSTANTIATE_llvm_densemap(K, C_KEY, V, C_VALUE)                        \
  typedef llvm::DenseMap<C_KEY, C_VALUE> K##_##V##_llvm_densemap_t;            \
  typedef K##_##V##_llvm_densemap_t *K##_##V##_llvm_densemap_p;                \
                                                                               \
  cname alwaysinline used                                                      \
      K##_##V##_llvm_densemap_p K##_##V##_llvm_densemap__allocate() {          \
    K##_##V##_llvm_densemap_p map = new K##_##V##_llvm_densemap_t();           \
    return map;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used void K##_##V##_llvm_densemap__free(                  \
      K##_##V##_llvm_densemap_p map) {                                         \
    delete map;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used bool K##_##V##_llvm_densemap__has(                   \
      K##_##V##_llvm_densemap_p map,                                           \
      C_KEY key) {                                                             \
    return map->count(key) > 0;                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used C_VALUE *K##_##V##_llvm_densemap__get(               \
      K##_##V##_llvm_densemap_p map,                                           \
      C_KEY key) {                                                             \
    return (C_VALUE *)(&((*map)[key]));                                        \
  }                                                                            \
                                                                               \
  cname alwaysinline used C_VALUE K##_##V##_llvm_densemap__read(               \
      K##_##V##_llvm_densemap_p map,                                           \
      C_KEY key) {                                                             \
    return (*map)[key];                                                        \
  }                                                                            \
                                                                               \
  cname alwaysinline used                                                      \
      K##_##V##_llvm_densemap_p K##_##V##_llvm_densemap__write(                \
          K##_##V##_llvm_densemap_p map,                                       \
          C_KEY key,                                                           \
          C_VALUE value) {                                                     \
    (*map)[key] = value;                                                       \
    return map;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used                                                      \
      K##_##V##_llvm_densemap_p K##_##V##_llvm_densemap__remove(               \
          K##_##V##_llvm_densemap_p map,                                       \
          C_KEY key) {                                                         \
    map->erase(key);                                                           \
    return map;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used size_t K##_##V##_llvm_densemap__size(                \
      K##_##V##_llvm_densemap_p map) {                                         \
    return map->size();                                                        \
  }

} // extern "C"
