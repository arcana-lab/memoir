// Simple hash table implemented in C.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define SMALL_MEMORY 15000000
#define BIG_MEMORY 150000000

#if !defined(TTABLE_SIZE)
#  define TTABLE_SIZE BIG_MEMORY
#endif

#define cname extern "C"
#define alwaysinline __attribute__((always_inline)) inline
#define used __attribute__((used))

extern "C" {

#define INSTANTIATE_deepsjeng_ttable(K, C_KEY, V, C_VALUE)                     \
  typedef C_VALUE *K##_##V##_deepsjeng_ttable_t;                               \
                                                                               \
  static K##_##V##_deepsjeng_ttable_t K##_##V##_ttable = NULL;                 \
                                                                               \
  cname alwaysinline used                                                      \
      K##_##V##_deepsjeng_ttable_t K##_##V##_deepsjeng_ttable__allocate(       \
          void) {                                                              \
    if (K##_##V##_ttable == NULL) {                                            \
      K##_##V##_ttable = (K##_##V##_deepsjeng_ttable_t)malloc(                 \
          (size_t)TTABLE_SIZE * sizeof(C_VALUE));                              \
    }                                                                          \
    memset(K##_##V##_ttable, 0, (size_t)TTABLE_SIZE * sizeof(C_VALUE));        \
    return K##_##V##_ttable;                                                   \
  }                                                                            \
                                                                               \
  cname alwaysinline used void K##_##V##_deepsjeng_ttable__free(               \
      K##_##V##_deepsjeng_ttable_t table) {                                    \
    memset(K##_##V##_ttable, 0, (size_t)TTABLE_SIZE * sizeof(C_VALUE));        \
  }                                                                            \
                                                                               \
  cname alwaysinline used bool K##_##V##_deepsjeng_ttable__has(                \
      K##_##V##_deepsjeng_ttable_t table,                                      \
      C_KEY key) {                                                             \
    return true;                                                               \
  }                                                                            \
                                                                               \
  /* Get key-value pair */                                                     \
  cname alwaysinline used C_VALUE *K##_##V##_deepsjeng_ttable__get(            \
      K##_##V##_deepsjeng_ttable_t table,                                      \
      C_KEY key) {                                                             \
    return &table[key];                                                        \
  }                                                                            \
                                                                               \
  cname alwaysinline used                                                      \
      K##_##V##_deepsjeng_ttable_t K##_##V##_deepsjeng_ttable__remove(         \
          K##_##V##_deepsjeng_ttable_t table,                                  \
          C_KEY key) {                                                         \
    memset(&table[key], 0, sizeof(C_VALUE));                                   \
    return table;                                                              \
  }                                                                            \
                                                                               \
  cname alwaysinline used size_t K##_##V##_deepsjeng_ttable__size(             \
      K##_##V##_deepsjeng_ttable_t table) {                                    \
    return TTABLE_SIZE;                                                        \
  }

} // extern "C"
