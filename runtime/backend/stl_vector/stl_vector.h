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

#define RESERVE_SIZE 4061

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
  cname alwaysinline used T##_stl_vector_p T##_stl_vector__allocate(           \
      size_t num) {                                                            \
    T##_stl_vector_p vec = new T##_stl_vector_t();                             \
    vec->resize(num);                                                          \
    if (num == 0) {                                                            \
      vec->reserve(RESERVE_SIZE);                                              \
    }                                                                          \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used void T##_stl_vector__free(T##_stl_vector_p vec) {    \
    delete vec;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used C_TYPE *T##_stl_vector__get(T##_stl_vector_p vec,    \
                                                      size_t index) {          \
    return (C_TYPE *)(&((*vec)[index]));                                       \
  }                                                                            \
                                                                               \
  cname alwaysinline used C_TYPE T##_stl_vector__read(T##_stl_vector_p vec,    \
                                                      size_t index) {          \
    return (*vec)[index];                                                      \
  }                                                                            \
                                                                               \
  cname alwaysinline used void T##_stl_vector__write(T##_stl_vector_p vec,     \
                                                     size_t index,             \
                                                     C_TYPE value) {           \
    (*vec)[index] = value;                                                     \
    return;                                                                    \
  }                                                                            \
                                                                               \
  cname alwaysinline used T##_stl_vector_p T##_stl_vector__copy(               \
      T##_stl_vector_p vec,                                                    \
      size_t begin_index,                                                      \
      size_t end_index) {                                                      \
    T##_stl_vector_p new_vec =                                                 \
        new T##_stl_vector_t(vec->cbegin() + begin_index,                      \
                             vec->cbegin() + end_index);                       \
    return new_vec;                                                            \
  }                                                                            \
                                                                               \
  cname alwaysinline used T##_stl_vector_p T##_stl_vector__remove(             \
      T##_stl_vector_p vec,                                                    \
      size_t index) {                                                          \
    vec->erase(vec->begin() + index);                                          \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used T##_stl_vector_p T##_stl_vector__remove_range(       \
      T##_stl_vector_p vec,                                                    \
      size_t begin_index,                                                      \
      size_t end_index) {                                                      \
    vec->erase(vec->begin() + begin_index, vec->begin() + end_index);          \
    return vec;                                                                \
  }                                                                            \
  cname alwaysinline used T##_stl_vector_p T##_stl_vector__insert(             \
      T##_stl_vector_p vec,                                                    \
      size_t start) {                                                          \
    using elem_type = C_TYPE;                                                  \
    vec->insert(vec->begin() + start, elem_type());                            \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used T##_stl_vector_p T##_stl_vector__insert_element(     \
      T##_stl_vector_p vec,                                                    \
      size_t start,                                                            \
      C_TYPE value) {                                                          \
    vec->insert(vec->begin() + start, value);                                  \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used T##_stl_vector_p T##_stl_vector__insert_range(       \
      T##_stl_vector_p vec,                                                    \
      size_t start,                                                            \
      T##_stl_vector_p vec2) {                                                 \
    vec->insert(vec->begin() + start, vec2->begin(), vec2->end());             \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used void T##_stl_vector__swap(T##_stl_vector_p vec,      \
                                                    size_t from,               \
                                                    size_t to,                 \
                                                    T##_stl_vector_p vec2,     \
                                                    size_t start) {            \
    std::swap_ranges(vec->begin() + from,                                      \
                     vec->begin() + to,                                        \
                     vec2->begin() + start);                                   \
    return;                                                                    \
  }                                                                            \
                                                                               \
  cname alwaysinline used size_t T##_stl_vector__size(T##_stl_vector_p vec) {  \
    return vec->size();                                                        \
  }                                                                            \
                                                                               \
  cname alwaysinline used void T##_stl_vector__begin(                          \
      T##_stl_vector_iter_p iter,                                              \
      T##_stl_vector_p vec) {                                                  \
    iter->_it = vec->begin();                                                  \
    iter->_ie = vec->end();                                                    \
    iter->_idx = 0;                                                            \
  }                                                                            \
  cname alwaysinline used bool T##_stl_vector__next(                           \
      T##_stl_vector_iter_p iter) {                                            \
    if (iter->_it == iter->_ie) {                                              \
      return false;                                                            \
    }                                                                          \
    ++iter->_idx;                                                              \
    iter->_val = *iter->_it;                                                   \
    ++iter->_it;                                                               \
    return true;                                                               \
  }                                                                            \
  cname alwaysinline used void T##_stl_vector__rbegin(                         \
      T##_stl_vector_riter_p iter,                                             \
      T##_stl_vector_p vec) {                                                  \
    iter->_it = vec->rbegin();                                                 \
    iter->_ie = vec->rend();                                                   \
    iter->_idx = vec->size() - 1;                                              \
  }                                                                            \
  cname alwaysinline used bool T##_stl_vector__rnext(                          \
      T##_stl_vector_iter_p iter) {                                            \
    if (iter->_it == iter->_ie) {                                              \
      return false;                                                            \
    }                                                                          \
    --iter->_idx;                                                              \
    iter->_val = *iter->_it;                                                   \
    ++iter->_it;                                                               \
    return true;                                                               \
  }

#define INSTANTIATE_NO_REF_stl_vector(T, C_TYPE)                               \
  typedef std::vector<C_TYPE> T##_stl_vector_t;                                \
  typedef T##_stl_vector_t *T##_stl_vector_p;                                  \
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
  cname alwaysinline used T##_stl_vector_p T##_stl_vector__allocate(           \
      size_t num) {                                                            \
    T##_stl_vector_p vec = new T##_stl_vector_t();                             \
    vec->resize(num);                                                          \
    if (num == 0) {                                                            \
      vec->reserve(RESERVE_SIZE);                                              \
    }                                                                          \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used void T##_stl_vector__free(T##_stl_vector_p vec) {    \
    delete vec;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used C_TYPE T##_stl_vector__read(T##_stl_vector_p vec,    \
                                                      size_t index) {          \
    return (*vec)[index];                                                      \
  }                                                                            \
                                                                               \
  cname alwaysinline used void T##_stl_vector__write(T##_stl_vector_p vec,     \
                                                     size_t index,             \
                                                     C_TYPE value) {           \
    (*vec)[index] = value;                                                     \
    return;                                                                    \
  }                                                                            \
                                                                               \
  cname alwaysinline used T##_stl_vector_p T##_stl_vector__copy(               \
      T##_stl_vector_p vec,                                                    \
      size_t begin_index,                                                      \
      size_t end_index) {                                                      \
    T##_stl_vector_p new_vec =                                                 \
        new T##_stl_vector_t(vec->cbegin() + begin_index,                      \
                             vec->cbegin() + end_index);                       \
    return new_vec;                                                            \
  }                                                                            \
                                                                               \
  cname alwaysinline used T##_stl_vector_p T##_stl_vector__remove(             \
      T##_stl_vector_p vec,                                                    \
      size_t index) {                                                          \
    vec->erase(vec->begin() + index);                                          \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used T##_stl_vector_p T##_stl_vector__remove_range(       \
      T##_stl_vector_p vec,                                                    \
      size_t begin_index,                                                      \
      size_t end_index) {                                                      \
    vec->erase(vec->begin() + begin_index, vec->begin() + end_index);          \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used T##_stl_vector_p T##_stl_vector__insert(             \
      T##_stl_vector_p vec,                                                    \
      size_t start) {                                                          \
    using elem_type = C_TYPE;                                                  \
    vec->insert(vec->begin() + start, elem_type());                            \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used T##_stl_vector_p T##_stl_vector__insert_element(     \
      T##_stl_vector_p vec,                                                    \
      size_t start,                                                            \
      C_TYPE value) {                                                          \
    vec->insert(vec->begin() + start, value);                                  \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used T##_stl_vector_p T##_stl_vector__insert_range(       \
      T##_stl_vector_p vec,                                                    \
      size_t start,                                                            \
      T##_stl_vector_p vec2) {                                                 \
    vec->insert(vec->begin() + start, vec2->begin(), vec2->end());             \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used void T##_stl_vector__swap(T##_stl_vector_p vec,      \
                                                    size_t from,               \
                                                    size_t to,                 \
                                                    T##_stl_vector_p vec2,     \
                                                    size_t start) {            \
    std::swap_ranges(vec->begin() + from,                                      \
                     vec->begin() + to,                                        \
                     vec2->begin() + start);                                   \
    return;                                                                    \
  }                                                                            \
                                                                               \
  cname alwaysinline used size_t T##_stl_vector__size(T##_stl_vector_p vec) {  \
    return vec->size();                                                        \
  }                                                                            \
                                                                               \
  cname alwaysinline used void T##_stl_vector__begin(                          \
      T##_stl_vector_iter_p iter,                                              \
      T##_stl_vector_p vec) {                                                  \
    iter->_it = vec->begin();                                                  \
    iter->_ie = vec->end();                                                    \
    iter->_idx = -1;                                                           \
  }                                                                            \
  cname alwaysinline used bool T##_stl_vector__next(                           \
      T##_stl_vector_iter_p iter) {                                            \
    if (iter->_it == iter->_ie) {                                              \
      return false;                                                            \
    }                                                                          \
    iter->_val = *iter->_it;                                                   \
    ++iter->_it;                                                               \
    ++iter->_idx;                                                              \
    return true;                                                               \
  }                                                                            \
  cname alwaysinline used void T##_stl_vector__rbegin(                         \
      T##_stl_vector_riter_p iter,                                             \
      T##_stl_vector_p vec) {                                                  \
    iter->_it = vec->rbegin();                                                 \
    iter->_ie = vec->rend();                                                   \
    iter->_idx = vec->size();                                                  \
  }                                                                            \
  cname alwaysinline used bool T##_stl_vector__rnext(                          \
      T##_stl_vector_iter_p iter) {                                            \
    if (iter->_it == iter->_ie) {                                              \
      return false;                                                            \
    }                                                                          \
    --iter->_idx;                                                              \
    iter->_val = *iter->_it;                                                   \
    ++iter->_it;                                                               \
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
  cname alwaysinline used T##_stl_vector_p T##_stl_vector__allocate(           \
      size_t num) {                                                            \
    T##_stl_vector_p vec = new T##_stl_vector_t();                             \
    vec->resize(num);                                                          \
    if (num == 0) {                                                            \
      vec->reserve(RESERVE_SIZE);                                              \
    }                                                                          \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used void T##_stl_vector__free(T##_stl_vector_p vec) {    \
    delete vec;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used C_TYPE *T##_stl_vector__get(T##_stl_vector_p vec,    \
                                                      size_t index) {          \
    return (C_TYPE *)(&((*vec)[index]));                                       \
  }                                                                            \
                                                                               \
  cname alwaysinline used C_TYPE T##_stl_vector__read(T##_stl_vector_p vec,    \
                                                      size_t index) {          \
    return (*vec)[index];                                                      \
  }                                                                            \
                                                                               \
  cname alwaysinline used void T##_stl_vector__write(T##_stl_vector_p vec,     \
                                                     size_t index,             \
                                                     C_TYPE value) {           \
    (*vec)[index] = value;                                                     \
    return;                                                                    \
  }                                                                            \
                                                                               \
  cname alwaysinline used T##_stl_vector_p T##_stl_vector__copy(               \
      T##_stl_vector_p vec,                                                    \
      size_t begin_index,                                                      \
      size_t end_index) {                                                      \
    T##_stl_vector_p new_vec =                                                 \
        new T##_stl_vector_t(vec->cbegin() + begin_index,                      \
                             vec->cbegin() + end_index);                       \
    return new_vec;                                                            \
  }                                                                            \
                                                                               \
  cname alwaysinline used T##_stl_vector_p T##_stl_vector__remove(             \
      T##_stl_vector_p vec,                                                    \
      size_t index) {                                                          \
    vec->erase(vec->begin() + index);                                          \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used T##_stl_vector_p T##_stl_vector__remove_range(       \
      T##_stl_vector_p vec,                                                    \
      size_t begin_index,                                                      \
      size_t end_index) {                                                      \
    vec->erase(vec->begin() + begin_index, vec->begin() + end_index);          \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used T##_stl_vector_p T##_stl_vector__insert(             \
      T##_stl_vector_p vec,                                                    \
      size_t start) {                                                          \
    using elem_type = C_TYPE;                                                  \
    vec->insert(vec->begin() + start, elem_type());                            \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used T##_stl_vector_p T##_stl_vector__insert_element(     \
      T##_stl_vector_p vec,                                                    \
      size_t start,                                                            \
      C_TYPE *value) {                                                         \
    vec->insert(vec->begin() + start, *value);                                 \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used T##_stl_vector_p T##_stl_vector__insert_range(       \
      T##_stl_vector_p vec,                                                    \
      size_t start,                                                            \
      T##_stl_vector_p vec2) {                                                 \
    vec->insert(vec->begin() + start, vec2->begin(), vec2->end());             \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used void T##_stl_vector__swap(T##_stl_vector_p vec,      \
                                                    size_t from,               \
                                                    size_t to,                 \
                                                    T##_stl_vector_p vec2,     \
                                                    size_t start) {            \
    std::swap_ranges(vec->begin() + from,                                      \
                     vec->begin() + to,                                        \
                     vec2->begin() + start);                                   \
    return;                                                                    \
  }                                                                            \
                                                                               \
  cname alwaysinline used size_t T##_stl_vector__size(T##_stl_vector_p vec) {  \
    return vec->size();                                                        \
  }                                                                            \
                                                                               \
  cname alwaysinline used void T##_stl_vector__begin(                          \
      T##_stl_vector_iter_p iter,                                              \
      T##_stl_vector_p vec) {                                                  \
    iter->_it = vec->begin();                                                  \
    iter->_ie = vec->end();                                                    \
    iter->_idx = -1;                                                           \
  }                                                                            \
  cname alwaysinline used bool T##_stl_vector__next(                           \
      T##_stl_vector_iter_p iter) {                                            \
    if (iter->_it == iter->_ie) {                                              \
      return false;                                                            \
    }                                                                          \
    ++iter->_idx;                                                              \
    iter->_val = *iter->_it;                                                   \
    ++iter->_it;                                                               \
    return true;                                                               \
  }                                                                            \
  cname alwaysinline used void T##_stl_vector__rbegin(                         \
      T##_stl_vector_riter_p iter,                                             \
      T##_stl_vector_p vec) {                                                  \
    iter->_it = vec->rbegin();                                                 \
    iter->_ie = vec->rend();                                                   \
    iter->_idx = vec->size();                                                  \
  }                                                                            \
  cname alwaysinline used bool T##_stl_vector__rnext(                          \
      T##_stl_vector_iter_p iter) {                                            \
    if (iter->_it == iter->_ie) {                                              \
      return false;                                                            \
    }                                                                          \
    --iter->_idx;                                                              \
    iter->_val = *iter->_it;                                                   \
    ++iter->_it;                                                               \
    return true;                                                               \
  }
