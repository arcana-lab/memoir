// Simple hash table implemented in C.
#include <cstdint>
#include <cstdio>

#include <type_traits>

#include <vector>

#define cname extern "C"
#define alwaysinline __attribute__((always_inline)) inline
#define used __attribute__((used))

#define K 4000
#define B 60
#define RESERVE_SIZE K + B + 1

extern "C" {

#define INSTANTIATE_stl_vector(T, C_TYPE)                                      \
  typedef std::vector<C_TYPE> T##_stl_vector_t;                                \
  typedef T##_stl_vector_t *T##_stl_vector_p;                                  \
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
  cname alwaysinline used T##_stl_vector_p T##_stl_vector__insert_element(     \
      T##_stl_vector_p vec,                                                    \
      size_t start,                                                            \
      C_TYPE value) {                                                          \
    vec->insert(vec->begin() + start, value);                                  \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used T##_stl_vector_p T##_stl_vector__insert(             \
      T##_stl_vector_p vec,                                                    \
      size_t start,                                                            \
      T##_stl_vector_p vec2) {                                                 \
    vec->insert(vec->begin() + start, vec2->begin(), vec2->end());             \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used T##_stl_vector_p T##_stl_vector__insert_range(       \
      T##_stl_vector_p vec,                                                    \
      size_t start,                                                            \
      T##_stl_vector_p vec2,                                                   \
      size_t from,                                                             \
      size_t to) {                                                             \
    vec->insert(vec->begin() + start,                                          \
                vec2->begin() + from,                                          \
                vec2->begin() + to);                                           \
    return vec;                                                                \
  }                                                                            \
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
  }

#define INSTANTIATE_NO_REF_stl_vector(T, C_TYPE)                               \
  typedef std::vector<C_TYPE> T##_stl_vector_t;                                \
  typedef T##_stl_vector_t *T##_stl_vector_p;                                  \
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
  cname alwaysinline used T##_stl_vector_p T##_stl_vector__insert_element(     \
      T##_stl_vector_p vec,                                                    \
      size_t start,                                                            \
      C_TYPE value) {                                                          \
    vec->insert(vec->begin() + start, value);                                  \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used T##_stl_vector_p T##_stl_vector__insert(             \
      T##_stl_vector_p vec,                                                    \
      size_t start,                                                            \
      T##_stl_vector_p vec2) {                                                 \
    vec->insert(vec->begin() + start, vec2->begin(), vec2->end());             \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  cname alwaysinline used T##_stl_vector_p T##_stl_vector__insert_range(       \
      T##_stl_vector_p vec,                                                    \
      size_t start,                                                            \
      T##_stl_vector_p vec2,                                                   \
      size_t from,                                                             \
      size_t to) {                                                             \
    vec->insert(vec->begin() + start,                                          \
                vec2->begin() + from,                                          \
                vec2->begin() + to);                                           \
    return vec;                                                                \
  }                                                                            \
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
  }

} // extern "C"
