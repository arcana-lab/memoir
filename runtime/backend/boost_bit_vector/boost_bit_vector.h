#include <cstdint>
#include <cstdio>

#include <type_traits>

#include <vector>

#include <functional>

#ifndef MEMOIR_BACKEND_CAT
#  define MEMOIR_BACKEND_CAT_(A, B) A##B
#  define MEMOIR_BACKEND_CAT(A, B) MEMOIR_BACKEND_CAT_(A, B)
#  define KEY_(_TYPE) MEMOIR_BACKEND_CAT(USING_KEY_, _TYPE)
#endif

#define cname extern "C"
#define alwaysinline __attribute__((always_inline)) inline
#define used __attribute__((used))

#ifndef FOLIO_boost_bit_vector
#  define FOLIO_boost_bit_vector

// An adapter for boost::dynamic_bitset to act more like std::vector.
struct BitVector {};

#endif

#undef INSTANTIATE_NO_REF_boost_bit_vector

#define INSTANTIATE_NO_REF_boost_bit_vector(T, C_TYPE)                         \
  typedef boost::dynamic_bitset<> T##_boost_bit_vector_t;                      \
  typedef T##_boost_bit_vector_t *T##_boost_bit_vector_p;                      \
  typedef struct T##_boost_bit_vector_iter {                                   \
    size_t _idx;                                                               \
    C_TYPE _val;                                                               \
    T##_boost_bit_vector_p _vec;                                               \
  } T##_boost_bit_vector_iter_t;                                               \
  typedef T##_boost_bit_vector_iter_t *T##_boost_bit_vector_iter_p;            \
                                                                               \
  typedef struct T##_boost_bit_vector_riter {                                  \
    size_t _idx;                                                               \
    C_TYPE _val;                                                               \
    T##_boost_bit_vector_p _vec;                                               \
  } T##_boost_bit_vector_riter_t;                                              \
  typedef T##_boost_bit_vector_riter_t *T##_boost_bit_vector_riter_p;          \
                                                                               \
  cname alwaysinline used                                                      \
      T##_boost_bit_vector_p T##_boost_bit_vector__allocate(size_t num) {      \
    T##_boost_bit_vector_p vec = new T##_boost_bit_vector_t(num);              \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used void T##_boost_bit_vector__free(                     \
      T##_boost_bit_vector_p vec) {                                            \
    delete vec;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used C_TYPE T##_boost_bit_vector__read(                   \
      T##_boost_bit_vector_p vec,                                              \
      size_t index) {                                                          \
    return vec->test(index);                                                   \
  }                                                                            \
                                                                               \
  cname alwaysinline used void T##_boost_bit_vector__write(                    \
      T##_boost_bit_vector_p vec,                                              \
      size_t index,                                                            \
      C_TYPE value) {                                                          \
    vec->set(index, value);                                                    \
    return;                                                                    \
  }                                                                            \
                                                                               \
  cname alwaysinline used size_t T##_boost_bit_vector__size(                   \
      T##_boost_bit_vector_p vec) {                                            \
    return vec->size();                                                        \
  }                                                                            \
                                                                               \
  cname alwaysinline used void T##_boost_bit_vector__begin(                    \
      T##_boost_bit_vector_iter_p iter,                                        \
      T##_boost_bit_vector_p vec) {                                            \
    iter->_vec = vec;                                                          \
    iter->_idx = -1;                                                           \
  }                                                                            \
  cname alwaysinline used bool T##_boost_bit_vector__next(                     \
      T##_boost_bit_vector_iter_p iter) {                                      \
    if (iter->_idx == vec->size()) {                                           \
      return false;                                                            \
    }                                                                          \
    iter->_val = vec->test(++iter->_idx);                                      \
    return true;                                                               \
  }                                                                            \
  cname alwaysinline used void T##_boost_bit_vector__rbegin(                   \
      T##_boost_bit_vector_riter_p iter,                                       \
      T##_boost_bit_vector_p vec) {                                            \
    iter->_vec = vec;                                                          \
    iter->_idx = vec->size();                                                  \
  }                                                                            \
  cname alwaysinline used bool T##_boost_bit_vector__rnext(                    \
      T##_boost_bit_vector_iter_p iter) {                                      \
    if (iter->_idx == 0) {                                                     \
      return false;                                                            \
    }                                                                          \
    iter->_val = vec->test(--iter->_idx);                                      \
    return true;                                                               \
  }

#undef INSTANTIATE_NESTED_boost_bit_vector
