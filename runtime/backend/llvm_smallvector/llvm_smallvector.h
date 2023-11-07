// Simple hash table implemented in C.
#include "llvm/ADT/SmallVector.h"

#include <cstdint>
#include <cstdio>

#define cname extern "C"
#define alwaysinline __attribute__((always_inline)) inline
#define used __attribute__((used))

#define SMALL_SIZE 256

extern "C" {

#define INSTANTIATE_llvm_smallvector(T, C_TYPE)                                \
  typedef struct T##_llvm_smallvector {                                        \
    T##_llvm_smallvector(size_t num) : _vec(num) {}                            \
    llvm::SmallVector<C_TYPE, SMALL_SIZE> _vec;                                \
  } T##_llvm_smallvector_t;                                                    \
  typedef T##_llvm_smallvector_t *T##_llvm_smallvector_p;                      \
                                                                               \
  cname alwaysinline used                                                      \
      T##_llvm_smallvector_p T##_llvm_smallvector__allocate(size_t num) {      \
    T##_llvm_smallvector_p vec = new T##_llvm_smallvector_t(num);              \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used                                                      \
      T##_llvm_smallvector_p T##_llvm_smallvector__initialize(                 \
          T##_llvm_smallvector_p rgn,                                          \
          size_t num) {                                                        \
    T##_llvm_smallvector_p vec = new (rgn) T##_llvm_smallvector_t(num);        \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used void T##_llvm_smallvector__free(                     \
      T##_llvm_smallvector_p vec) {                                            \
    delete vec;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used C_TYPE *T##_llvm_smallvector__get(                   \
      T##_llvm_smallvector_p vec,                                              \
      size_t index) {                                                          \
    return (C_TYPE *)(&(vec->_vec[index]));                                    \
  }                                                                            \
                                                                               \
  cname alwaysinline used C_TYPE T##_llvm_smallvector__read(                   \
      T##_llvm_smallvector_p vec,                                              \
      size_t index) {                                                          \
    return (vec->_vec)[index];                                                 \
  }                                                                            \
                                                                               \
  cname alwaysinline used void T##_llvm_smallvector__write(                    \
      T##_llvm_smallvector_p vec,                                              \
      size_t index,                                                            \
      C_TYPE value) {                                                          \
    (vec->_vec)[index] = value;                                                \
    return;                                                                    \
  }                                                                            \
                                                                               \
  cname alwaysinline used T##_llvm_smallvector_p T##_llvm_smallvector__copy(   \
      T##_llvm_smallvector_p vec,                                              \
      size_t begin_index,                                                      \
      size_t end_index) {                                                      \
    return new T##_llvm_smallvector_t(vec->begin() + begin_index,              \
                                      vec->begin() + end_index);               \
  }                                                                            \
                                                                               \
  cname alwaysinline used T##_llvm_smallvector_p T##_llvm_smallvector__remove( \
      T##_llvm_smallvector_p vec,                                              \
      size_t index) {                                                          \
    vec->_vec.erase(vec->_vec.begin() + index);                                \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used                                                      \
      T##_llvm_smallvector_p T##_llvm_smallvector__remove_range(               \
          T##_llvm_smallvector_p vec,                                          \
          size_t begin_index,                                                  \
          size_t end_index) {                                                  \
    vec->_vec.erase(vec->_vec.begin() + begin_index,                           \
                    vec->_vec.begin() + end_index);                            \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used                                                      \
      T##_llvm_smallvector_p T##_llvm_smallvector__insert_element(             \
          T##_llvm_smallvector_p vec,                                          \
          size_t start,                                                        \
          C_TYPE value) {                                                      \
    vec->insert(vec->begin() + start, value);                                  \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used T##_llvm_smallvector_p T##_llvm_smallvector__insert( \
      T##_llvm_smallvector_p vec,                                              \
      size_t start,                                                            \
      T##_llvm_smallvector_p vec2) {                                           \
    vec->_vec.insert(vec->_vec.begin() + start,                                \
                     vec2->_vec.begin(),                                       \
                     vec2->_vec.end());                                        \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used                                                      \
      T##_llvm_smallvector_p T##_llvm_smallvector__insert_range(               \
          T##_llvm_smallvector_p vec,                                          \
          size_t start,                                                        \
          T##_llvm_smallvector_p vec2,                                         \
          size_t from,                                                         \
          size_t to) {                                                         \
    vec->_vec.insert(vec->_vec.begin() + start,                                \
                     vec2->_vec.begin() + from,                                \
                     vec2->_vec.begin() + to);                                 \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used void T##_llvm_smallvector__swap(                     \
      T##_llvm_smallvector_p vec,                                              \
      size_t from,                                                             \
      size_t to,                                                               \
      T##_llvm_smallvector_p vec2,                                             \
      size_t start) {                                                          \
    std::swap_ranges(vec->begin() + from,                                      \
                     vec->begin() + to,                                        \
                     vec2->begin() + start);                                   \
    return;                                                                    \
  }                                                                            \
                                                                               \
  cname alwaysinline used size_t T##_llvm_smallvector__size(                   \
      T##_llvm_smallvector_p vec) {                                            \
    return vec->_vec.size();                                                   \
  }

} // extern "C"
