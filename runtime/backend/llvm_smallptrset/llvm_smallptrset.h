// Simple hash table implemented in C.
#include "llvm/ADT/SmallPtrSet.h"

#include <cstdint>
#include <cstdio>

#define cname extern "C"
#define alwaysinline __attribute__((always_inline)) inline
#define used __attribute__((used))

#define SMALL_SIZE 8

extern "C" {

#define INSTANTIATE_llvm_smallptrset(K, C_KEY, V, C_VALUE)                     \
  typedef struct K##_##V##_llvm_smallptrset_t {                                \
    llvm::SmallPtrSet<C_KEY, SMALL_SIZE> _set;                                 \
  } K##_##V##_llvm_smallptrset_t;                                              \
  typedef K##_##V##_llvm_smallptrset_t *K##_##V##_llvm_smallptrset_p;          \
                                                                               \
  cname alwaysinline used                                                      \
      K##_##V##_llvm_smallptrset_p K##_##V##_llvm_smallptrset__allocate() {    \
    K##_##V##_llvm_smallptrset_p set = new K##_##V##_llvm_smallptrset_t();     \
    return set;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used                                                      \
      K##_##V##_llvm_smallptrset_p K##_##V##_llvm_smallptrset__initialize(     \
          K##_##V##_llvm_smallptrset_p rgn) {                                  \
    K##_##V##_llvm_smallptrset_p set =                                         \
        new (rgn) K##_##V##_llvm_smallptrset_t();                              \
    return set;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used void K##_##V##_llvm_smallptrset__free(               \
      K##_##V##_llvm_smallptrset_p set) {                                      \
    set->~K##_##V##_llvm_smallptrset_t();                                      \
    return;                                                                    \
  }                                                                            \
                                                                               \
  cname alwaysinline used C_VALUE K##_##V##_llvm_smallptrset__has(             \
      K##_##V##_llvm_smallptrset_p set,                                        \
      C_KEY key) {                                                             \
    return set->_set.count(key) > 0;                                           \
  }                                                                            \
                                                                               \
  cname alwaysinline used                                                      \
      K##_##V##_llvm_smallptrset_p K##_##V##_llvm_smallptrset__write(          \
          K##_##V##_llvm_smallptrset_p set,                                    \
          C_KEY key,                                                           \
          C_VALUE value) {                                                     \
    set->_set.insert(key);                                                     \
    return set;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used                                                      \
      K##_##V##_llvm_smallptrset_p K##_##V##_llvm_smallptrset__remove(         \
          K##_##V##_llvm_smallptrset_p set,                                    \
          C_KEY key) {                                                         \
    set->_set.erase(key);                                                      \
    return set;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used size_t K##_##V##_llvm_smallptrset__size(             \
      K##_##V##_llvm_smallptrset_p set) {                                      \
    return set->_set.size();                                                   \
  }

} // extern "C"
