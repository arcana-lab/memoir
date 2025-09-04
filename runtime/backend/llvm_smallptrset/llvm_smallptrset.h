// Simple hash table implemented in C.
#include "llvm/ADT/SmallPtrSet.h"

#include <cstdint>
#include <cstdio>

#define CNAME extern "C"
#define ALWAYS_INLINE __attribute__((always_inline)) inline
#define USED __attribute__((USED))

#define SMALL_SIZE 8

extern "C" {

#define INSTANTIATE_llvm_smallptrset(K, C_KEY, V, C_VALUE)                     \
  typedef struct K##_##V##_llvm_smallptrset {                                  \
    llvm::SmallPtrSet<C_KEY, SMALL_SIZE> _set;                                 \
  } K##_##V##_llvm_smallptrset_t;                                              \
  typedef K##_##V##_llvm_smallptrset_t *K##_##V##_llvm_smallptrset_p;          \
                                                                               \
  CNAME ALWAYS_INLINE USED                                                     \
      K##_##V##_llvm_smallptrset_p K##_##V##_llvm_smallptrset__allocate() {    \
    K##_##V##_llvm_smallptrset_p set = new K##_##V##_llvm_smallptrset_t();     \
    return set;                                                                \
  }                                                                            \
                                                                               \
  CNAME ALWAYS_INLINE USED                                                     \
      K##_##V##_llvm_smallptrset_p K##_##V##_llvm_smallptrset__initialize(     \
          K##_##V##_llvm_smallptrset_p rgn) {                                  \
    K##_##V##_llvm_smallptrset_p set =                                         \
        new (rgn) K##_##V##_llvm_smallptrset_t();                              \
    return set;                                                                \
  }                                                                            \
                                                                               \
  CNAME ALWAYS_INLINE USED void K##_##V##_llvm_smallptrset__free(              \
      K##_##V##_llvm_smallptrset_p set) {                                      \
    delete set;                                                                \
    return;                                                                    \
  }                                                                            \
                                                                               \
  CNAME ALWAYS_INLINE USED C_VALUE K##_##V##_llvm_smallptrset__has(            \
      K##_##V##_llvm_smallptrset_p set,                                        \
      C_KEY key) {                                                             \
    return set->_set.count(key) > 0;                                           \
  }                                                                            \
                                                                               \
  CNAME ALWAYS_INLINE USED                                                     \
      K##_##V##_llvm_smallptrset_p K##_##V##_llvm_smallptrset__write(          \
          K##_##V##_llvm_smallptrset_p set,                                    \
          C_KEY key,                                                           \
          C_VALUE value) {                                                     \
    set->_set.insert(key);                                                     \
    return set;                                                                \
  }                                                                            \
                                                                               \
  CNAME ALWAYS_INLINE USED                                                     \
      K##_##V##_llvm_smallptrset_p K##_##V##_llvm_smallptrset__remove(         \
          K##_##V##_llvm_smallptrset_p set,                                    \
          C_KEY key) {                                                         \
    set->_set.erase(key);                                                      \
    return set;                                                                \
  }                                                                            \
                                                                               \
  CNAME ALWAYS_INLINE USED size_t K##_##V##_llvm_smallptrset__size(            \
      K##_##V##_llvm_smallptrset_p set) {                                      \
    return set->_set.size();                                                   \
  }

} // extern "C"
