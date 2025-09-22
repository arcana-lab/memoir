#include <cstdint>
#include <cstdio>

#include <type_traits>

#include <vector>

#include <functional>

#include <boost/dynamic_bitset.hpp>

#ifndef MEMOIR_BACKEND_CAT
#  define MEMOIR_BACKEND_CAT_(A, B) A##B
#  define MEMOIR_BACKEND_CAT(A, B) MEMOIR_BACKEND_CAT_(A, B)
#  define KEY_(_TYPE) MEMOIR_BACKEND_CAT(USING_KEY_, _TYPE)
#endif

#define CNAME extern "C"
#define ALWAYS_INLINE __attribute__((always_inline)) inline
#define USED __attribute__((USED))

#define RESERVE_SIZE 4000 + 60 + 1

#define INSTANTIATE_stl_vector(T, C_TYPE)                                      \
  typedef std::vector<C_TYPE> T##_stl_vector_t;                                \
  typedef T##_stl_vector_t *T##_stl_vector_p;                                  \
                                                                               \
  typedef struct T##_stl_vector_iter {                                         \
    size_t _idx;                                                               \
    C_TYPE _val;                                                               \
    std::vector<C_TYPE>::iterator _it;                                         \
    std::vector<C_TYPE>::iterator _ie;                                         \
  } T##_stl_vector_iter_t;                                                     \
  typedef T##_stl_vector_iter_t *T##_stl_vector_iter_p;                        \
                                                                               \
  typedef struct T##_stl_vector_riter {                                        \
    size_t _idx;                                                               \
    C_TYPE _val;                                                               \
    std::vector<C_TYPE>::reverse_iterator _it;                                 \
    std::vector<C_TYPE>::reverse_iterator _ie;                                 \
  } T##_stl_vector_riter_t;                                                    \
  typedef T##_stl_vector_riter_t *T##_stl_vector_riter_p;                      \
                                                                               \
  CNAME ALWAYS_INLINE USED T##_stl_vector_p T##_stl_vector__allocate(          \
      size_t num) {                                                            \
    T##_stl_vector_p vec = new T##_stl_vector_t();                             \
    vec->resize(num);                                                          \
    if (num == 0) {                                                            \
      vec->reserve(RESERVE_SIZE);                                              \
    }                                                                          \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  CNAME ALWAYS_INLINE USED void T##_stl_vector__free(T##_stl_vector_p vec) {   \
    delete vec;                                                                \
  }                                                                            \
                                                                               \
  CNAME ALWAYS_INLINE USED C_TYPE *T##_stl_vector__get(T##_stl_vector_p vec,   \
                                                       size_t index) {         \
    return (C_TYPE *)(&((*vec)[index]));                                       \
  }                                                                            \
                                                                               \
  CNAME ALWAYS_INLINE USED C_TYPE T##_stl_vector__read(T##_stl_vector_p vec,   \
                                                       size_t index) {         \
    return (*vec)[index];                                                      \
  }                                                                            \
                                                                               \
  CNAME ALWAYS_INLINE USED T##_stl_vector_p T##_stl_vector__write(             \
      T##_stl_vector_p vec,                                                    \
      size_t index,                                                            \
      C_TYPE value) {                                                          \
    (*vec)[index] = value;                                                     \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  CNAME ALWAYS_INLINE USED T##_stl_vector_p T##_stl_vector__copy(              \
      T##_stl_vector_p vec) {                                                  \
    T##_stl_vector_p new_vec =                                                 \
        new T##_stl_vector_t(vec->cbegin(), vec->cend());                      \
    return new_vec;                                                            \
  }                                                                            \
                                                                               \
  CNAME ALWAYS_INLINE USED T##_stl_vector_p T##_stl_vector__copy_range(        \
      T##_stl_vector_p vec,                                                    \
      size_t begin_index,                                                      \
      size_t end_index) {                                                      \
    T##_stl_vector_p new_vec =                                                 \
        new T##_stl_vector_t(std::next(vec->cbegin(), begin_index),            \
                             std::next(vec->cbegin(), end_index));             \
    return new_vec;                                                            \
  }                                                                            \
                                                                               \
  CNAME ALWAYS_INLINE USED T##_stl_vector_p T##_stl_vector__remove(            \
      T##_stl_vector_p vec,                                                    \
      size_t index) {                                                          \
    vec->erase(std::next(vec->begin(), index));                                \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  CNAME ALWAYS_INLINE USED T##_stl_vector_p T##_stl_vector__remove_range(      \
      T##_stl_vector_p vec,                                                    \
      size_t begin_index,                                                      \
      size_t end_index) {                                                      \
    vec->erase(std::next(vec->begin(), begin_index),                           \
               std::next(vec->begin(), end_index));                            \
    return vec;                                                                \
  }                                                                            \
  CNAME ALWAYS_INLINE USED T##_stl_vector_p T##_stl_vector__insert(            \
      T##_stl_vector_p vec,                                                    \
      size_t start) {                                                          \
    using elem_type = C_TYPE;                                                  \
    vec->insert(std::next(vec->begin(), start), elem_type());                  \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  CNAME ALWAYS_INLINE USED T##_stl_vector_p T##_stl_vector__insert_value(      \
      T##_stl_vector_p vec,                                                    \
      size_t start,                                                            \
      C_TYPE value) {                                                          \
    vec->insert(std::next(vec->begin(), start), value);                        \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  CNAME ALWAYS_INLINE USED T##_stl_vector_p T##_stl_vector__insert_input(      \
      T##_stl_vector_p vec,                                                    \
      size_t start,                                                            \
      T##_stl_vector_p vec2) {                                                 \
    vec->insert(std::next(vec->begin(), start), vec2->cbegin(), vec2->cend()); \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  CNAME ALWAYS_INLINE USED                                                     \
      T##_stl_vector_p T##_stl_vector__insert_input_range(                     \
          T##_stl_vector_p vec,                                                \
          size_t start,                                                        \
          T##_stl_vector_p vec2,                                               \
          size_t begin,                                                        \
          size_t end) {                                                        \
    vec->insert(std::next(vec->begin(), start),                                \
                std::next(vec2->cbegin(), begin),                              \
                std::next(vec2->cbegin(), end));                               \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  CNAME ALWAYS_INLINE USED size_t T##_stl_vector__size(T##_stl_vector_p vec) { \
    return vec->size();                                                        \
  }                                                                            \
                                                                               \
  CNAME ALWAYS_INLINE USED T##_stl_vector_p T##_stl_vector__clear(             \
      T##_stl_vector_p vec) {                                                  \
    vec->clear();                                                              \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  CNAME ALWAYS_INLINE USED void T##_stl_vector__begin(                         \
      T##_stl_vector_iter_p iter,                                              \
      T##_stl_vector_p vec) {                                                  \
    iter->_it = vec->begin();                                                  \
    iter->_ie = vec->end();                                                    \
    iter->_idx = -1;                                                           \
  }                                                                            \
  CNAME ALWAYS_INLINE USED bool T##_stl_vector__next(                          \
      T##_stl_vector_iter_p iter) {                                            \
    if (iter->_it == iter->_ie) {                                              \
      return false;                                                            \
    }                                                                          \
    ++iter->_idx;                                                              \
    iter->_val = *iter->_it;                                                   \
    ++iter->_it;                                                               \
    return true;                                                               \
  }                                                                            \
  CNAME ALWAYS_INLINE USED void T##_stl_vector__rbegin(                        \
      T##_stl_vector_riter_p iter,                                             \
      T##_stl_vector_p vec) {                                                  \
    iter->_it = vec->rbegin();                                                 \
    iter->_ie = vec->rend();                                                   \
    iter->_idx = vec->size();                                                  \
  }                                                                            \
  CNAME ALWAYS_INLINE USED bool T##_stl_vector__rnext(                         \
      T##_stl_vector_riter_p iter) {                                           \
    if (iter->_it == iter->_ie) {                                              \
      return false;                                                            \
    }                                                                          \
    --iter->_idx;                                                              \
    iter->_val = *iter->_it;                                                   \
    ++iter->_it;                                                               \
    return true;                                                               \
  }

#define INSTANTIATE_NO_REF_stl_vector(T, C_TYPE)                               \
  typedef boost::dynamic_bitset<> T##_stl_vector_t;                            \
  typedef T##_stl_vector_t *T##_stl_vector_p;                                  \
  typedef struct T##_stl_vector_iter {                                         \
    size_t _idx;                                                               \
    C_TYPE _val;                                                               \
    T##_stl_vector_p _vec;                                                     \
  } T##_stl_vector_iter_t;                                                     \
  typedef T##_stl_vector_iter_t *T##_stl_vector_iter_p;                        \
                                                                               \
  typedef struct T##_stl_vector_riter {                                        \
    size_t _idx;                                                               \
    C_TYPE _val;                                                               \
    T##_stl_vector_p _vec;                                                     \
  } T##_stl_vector_riter_t;                                                    \
  typedef T##_stl_vector_riter_t *T##_stl_vector_riter_p;                      \
                                                                               \
  CNAME ALWAYS_INLINE USED T##_stl_vector_p T##_stl_vector__allocate(          \
      size_t num) {                                                            \
    T##_stl_vector_p vec = new T##_stl_vector_t(num);                          \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  CNAME ALWAYS_INLINE USED void T##_stl_vector__free(T##_stl_vector_p vec) {   \
    delete vec;                                                                \
  }                                                                            \
                                                                               \
  CNAME ALWAYS_INLINE USED C_TYPE T##_stl_vector__read(T##_stl_vector_p vec,   \
                                                       size_t index) {         \
    return vec->test(index);                                                   \
  }                                                                            \
                                                                               \
  CNAME ALWAYS_INLINE USED T##_stl_vector_p T##_stl_vector__write(             \
      T##_stl_vector_p vec,                                                    \
      size_t index,                                                            \
      C_TYPE value) {                                                          \
    vec->set(index, value);                                                    \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  CNAME ALWAYS_INLINE USED size_t T##_stl_vector__size(T##_stl_vector_p vec) { \
    return vec->size();                                                        \
  }                                                                            \
                                                                               \
  CNAME ALWAYS_INLINE USED void T##_stl_vector__begin(                         \
      T##_stl_vector_iter_p iter,                                              \
      T##_stl_vector_p vec) {                                                  \
    iter->_vec = vec;                                                          \
    iter->_idx = -1;                                                           \
  }                                                                            \
  CNAME ALWAYS_INLINE USED bool T##_stl_vector__next(                          \
      T##_stl_vector_iter_p iter) {                                            \
    if (iter->_idx == iter->_vec->size()) {                                    \
      return false;                                                            \
    }                                                                          \
    iter->_val = iter->_vec->test(++iter->_idx);                               \
    return true;                                                               \
  }                                                                            \
  CNAME ALWAYS_INLINE USED void T##_stl_vector__rbegin(                        \
      T##_stl_vector_riter_p iter,                                             \
      T##_stl_vector_p vec) {                                                  \
    iter->_vec = vec;                                                          \
    iter->_idx = vec->size();                                                  \
  }                                                                            \
  CNAME ALWAYS_INLINE USED bool T##_stl_vector__rnext(                         \
      T##_stl_vector_iter_p iter) {                                            \
    if (iter->_idx == 0) {                                                     \
      return false;                                                            \
    }                                                                          \
    iter->_val = iter->_vec->test(--iter->_idx);                               \
    return true;                                                               \
  }

#define INSTANTIATE_NESTED_stl_vector(T, C_TYPE)                               \
  typedef std::vector<C_TYPE> T##_stl_vector_t;                                \
  typedef T##_stl_vector_t *T##_stl_vector_p;                                  \
                                                                               \
  typedef struct T##_stl_vector_iter {                                         \
    size_t _idx;                                                               \
    C_TYPE _val;                                                               \
    std::vector<C_TYPE>::iterator _it;                                         \
    std::vector<C_TYPE>::iterator _ie;                                         \
  } T##_stl_vector_iter_t;                                                     \
  typedef T##_stl_vector_iter_t *T##_stl_vector_iter_p;                        \
                                                                               \
  typedef struct T##_stl_vector_riter {                                        \
    size_t _idx;                                                               \
    C_TYPE _val;                                                               \
    std::vector<C_TYPE>::reverse_iterator _it;                                 \
    std::vector<C_TYPE>::reverse_iterator _ie;                                 \
    char _padding;                                                             \
  } T##_stl_vector_riter_t;                                                    \
  typedef T##_stl_vector_riter_t *T##_stl_vector_riter_p;                      \
                                                                               \
  CNAME ALWAYS_INLINE USED T##_stl_vector_p T##_stl_vector__allocate(          \
      size_t num) {                                                            \
    T##_stl_vector_p vec = new T##_stl_vector_t();                             \
    vec->resize(num);                                                          \
    if (num == 0) {                                                            \
      vec->reserve(RESERVE_SIZE);                                              \
    }                                                                          \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  CNAME ALWAYS_INLINE USED void T##_stl_vector__free(T##_stl_vector_p vec) {   \
    delete vec;                                                                \
  }                                                                            \
                                                                               \
  CNAME ALWAYS_INLINE USED C_TYPE *T##_stl_vector__get(T##_stl_vector_p vec,   \
                                                       size_t index) {         \
    return (C_TYPE *)(&((*vec)[index]));                                       \
  }                                                                            \
                                                                               \
  CNAME ALWAYS_INLINE USED C_TYPE T##_stl_vector__read(T##_stl_vector_p vec,   \
                                                       size_t index) {         \
    return (*vec)[index];                                                      \
  }                                                                            \
                                                                               \
  CNAME ALWAYS_INLINE USED T##_stl_vector_p T##_stl_vector__write(             \
      T##_stl_vector_p vec,                                                    \
      size_t index,                                                            \
      C_TYPE value) {                                                          \
    (*vec)[index] = value;                                                     \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  CNAME ALWAYS_INLINE USED T##_stl_vector_p T##_stl_vector__copy(              \
      T##_stl_vector_p vec) {                                                  \
    T##_stl_vector_p new_vec =                                                 \
        new T##_stl_vector_t(vec->cbegin(), vec->cend());                      \
    return new_vec;                                                            \
  }                                                                            \
                                                                               \
  CNAME ALWAYS_INLINE USED T##_stl_vector_p T##_stl_vector__copy_range(        \
      T##_stl_vector_p vec,                                                    \
      size_t begin_index,                                                      \
      size_t end_index) {                                                      \
    T##_stl_vector_p new_vec =                                                 \
        new T##_stl_vector_t(std::next(vec->cbegin(), begin_index),            \
                             std::next(vec->cbegin(), end_index));             \
    return new_vec;                                                            \
  }                                                                            \
                                                                               \
  CNAME ALWAYS_INLINE USED T##_stl_vector_p T##_stl_vector__remove(            \
      T##_stl_vector_p vec,                                                    \
      size_t index) {                                                          \
    vec->erase(std::next(vec->begin(), index));                                \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  CNAME ALWAYS_INLINE USED T##_stl_vector_p T##_stl_vector__remove_range(      \
      T##_stl_vector_p vec,                                                    \
      size_t begin_index,                                                      \
      size_t end_index) {                                                      \
    vec->erase(std::next(vec->begin(), begin_index),                           \
               std::next(vec->begin(), end_index));                            \
    return vec;                                                                \
  }                                                                            \
  CNAME ALWAYS_INLINE USED T##_stl_vector_p T##_stl_vector__insert(            \
      T##_stl_vector_p vec,                                                    \
      size_t start) {                                                          \
    using elem_type = C_TYPE;                                                  \
    vec->insert(std::next(vec->begin(), start), elem_type());                  \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  CNAME ALWAYS_INLINE USED T##_stl_vector_p T##_stl_vector__insert_value(      \
      T##_stl_vector_p vec,                                                    \
      size_t start,                                                            \
      C_TYPE value) {                                                          \
    vec->insert(std::next(vec->begin(), start), value);                        \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  CNAME ALWAYS_INLINE USED T##_stl_vector_p T##_stl_vector__insert_input(      \
      T##_stl_vector_p vec,                                                    \
      size_t start,                                                            \
      T##_stl_vector_p vec2) {                                                 \
    vec->insert(std::next(vec->begin(), start), vec2->cbegin(), vec2->cend()); \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  CNAME ALWAYS_INLINE USED                                                     \
      T##_stl_vector_p T##_stl_vector__insert_input_range(                     \
          T##_stl_vector_p vec,                                                \
          size_t start,                                                        \
          T##_stl_vector_p vec2,                                               \
          size_t begin,                                                        \
          size_t end) {                                                        \
    vec->insert(std::next(vec->begin(), start),                                \
                std::next(vec2->cbegin(), begin),                              \
                std::next(vec2->cbegin(), end));                               \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  CNAME ALWAYS_INLINE USED size_t T##_stl_vector__size(T##_stl_vector_p vec) { \
    return vec->size();                                                        \
  }                                                                            \
                                                                               \
  CNAME ALWAYS_INLINE USED void T##_stl_vector__begin(                         \
      T##_stl_vector_iter_p iter,                                              \
      T##_stl_vector_p vec) {                                                  \
    iter->_it = vec->begin();                                                  \
    iter->_ie = vec->end();                                                    \
    iter->_idx = -1;                                                           \
  }                                                                            \
  CNAME ALWAYS_INLINE USED bool T##_stl_vector__next(                          \
      T##_stl_vector_iter_p iter) {                                            \
    if (iter->_it == iter->_ie) {                                              \
      return false;                                                            \
    }                                                                          \
    ++iter->_idx;                                                              \
    iter->_val = *iter->_it;                                                   \
    ++iter->_it;                                                               \
    return true;                                                               \
  }                                                                            \
  CNAME ALWAYS_INLINE USED void T##_stl_vector__rbegin(                        \
      T##_stl_vector_riter_p iter,                                             \
      T##_stl_vector_p vec) {                                                  \
    iter->_it = vec->rbegin();                                                 \
    iter->_ie = vec->rend();                                                   \
    iter->_idx = vec->size();                                                  \
  }                                                                            \
  CNAME ALWAYS_INLINE USED bool T##_stl_vector__rnext(                         \
      T##_stl_vector_riter_p iter) {                                           \
    if (iter->_it == iter->_ie) {                                              \
      return false;                                                            \
    }                                                                          \
    --iter->_idx;                                                              \
    iter->_val = *iter->_it;                                                   \
    ++iter->_it;                                                               \
    return true;                                                               \
  }
