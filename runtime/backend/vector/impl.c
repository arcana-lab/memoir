#include "float.h"
#include "stdbool.h"
#include "stddef.h"
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#define alwaysinline __attribute__((always_inline)) inline
#define used __attribute__((used))

#if defined(__cplusplus)
extern "C" {
#endif

#define INSTANTIATE_TYPED_VECTOR(T, C_TYPE)                                    \
  typedef struct T##_vector {                                                  \
    size_t _front;                                                             \
    size_t _back;                                                              \
    size_t _max_size;                                                          \
    C_TYPE _storage[];                                                         \
  } T##_vector_t;                                                              \
  typedef T##_vector_t *T##_vector_p;                                          \
                                                                               \
  static alwaysinline used T##_vector_p T##_vector__allocate(size_t n) {       \
    size_t next_pow_2 = n - 1;                                                 \
    next_pow_2 |= next_pow_2 >> 1;                                             \
    next_pow_2 |= next_pow_2 >> 2;                                             \
    next_pow_2 |= next_pow_2 >> 4;                                             \
    next_pow_2 |= next_pow_2 >> 8;                                             \
    next_pow_2 |= next_pow_2 >> 16;                                            \
    next_pow_2 |= next_pow_2 >> 32;                                            \
    next_pow_2++;                                                              \
                                                                               \
    T##_vector_p alloc = (T##_vector_p)malloc(                                 \
        sizeof(T##_vector_t) + (next_pow_2 * sizeof(C_TYPE)));                 \
                                                                               \
    size_t diff = next_pow_2 - n;                                              \
    alloc->_front = diff / 2;                                                  \
    alloc->_back = alloc->_front + n;                                          \
    alloc->_max_size = next_pow_2;                                             \
                                                                               \
    return alloc;                                                              \
  }                                                                            \
                                                                               \
  static alwaysinline used void T##_vector__free(T##_vector_p vec) {           \
    free(vec);                                                                 \
    return;                                                                    \
  }                                                                            \
                                                                               \
  static alwaysinline used size_t T##_vector__size(T##_vector_p vec) {         \
    return vec->_back - vec->_front;                                           \
  }                                                                            \
                                                                               \
  static alwaysinline used C_TYPE T##_vector__read(const T##_vector_p vec,     \
                                                   size_t index) {             \
    return vec->_storage[vec->_front + index];                                 \
  }                                                                            \
                                                                               \
  static alwaysinline used void T##_vector__write(T##_vector_p vec,            \
                                                  size_t index,                \
                                                  C_TYPE value) {              \
    vec->_storage[vec->_front + index] = value;                                \
    return;                                                                    \
  }                                                                            \
                                                                               \
  static alwaysinline used T##_vector_p T##_vector__remove(T##_vector_p vec,   \
                                                           size_t from,        \
                                                           size_t to) {        \
    size_t current_size = vec->_back - vec->_front;                            \
    size_t removal_size = to - from;                                           \
    if (to == current_size) {                                                  \
      vec->_back -= removal_size;                                              \
    } else if (from == 0) {                                                    \
      vec->_front += removal_size;                                             \
    } else {                                                                   \
      size_t remove_front = from;                                              \
      size_t remove_back = current_size - to;                                  \
      /* If there is less to remove from the back. */                          \
      if (remove_back < remove_front) {                                        \
        /* _back - (from-to)  ==> _front+from      */                          \
        memmove((void *)&vec->_storage[vec->_front + from],                    \
                (const void *)&vec->_storage[vec->_back - (from - to)],        \
                remove_back);                                                  \
        vec->_back -= to - from;                                               \
      } /* Otherwise, let's remove from the front.*/                           \
      else {                                                                   \
        /* _front+from  ==> to */                                              \
        memmove((void *)&vec->_storage[vec->_front + to - from],               \
                (const void *)&vec->_storage[vec->_front],                     \
                sizeof(C_TYPE) * remove_front);                                \
        vec->_front += to - from;                                              \
      }                                                                        \
    }                                                                          \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  static alwaysinline used T##_vector_p T##_vector__insert_range(              \
      T##_vector_p vec,                                                        \
      size_t start,                                                            \
      T##_vector_p vec2,                                                       \
      size_t from,                                                             \
      size_t to) {                                                             \
    size_t vec_size = vec->_back - vec->_front;                                \
    size_t insert_size = to - from;                                            \
    size_t size_after_insert = vec_size + insert_size;                         \
                                                                               \
    bool insert_to_back = (start > 0);                                         \
                                                                               \
    if (size_after_insert > vec->_max_size) {                                  \
      /* grow the T##_vector. */                                               \
      size_t next_pow_2 = vec_size + insert_size;                              \
      next_pow_2 |= next_pow_2 >> 1;                                           \
      next_pow_2 |= next_pow_2 >> 2;                                           \
      next_pow_2 |= next_pow_2 >> 4;                                           \
      next_pow_2 |= next_pow_2 >> 8;                                           \
      next_pow_2 |= next_pow_2 >> 16;                                          \
      next_pow_2 |= next_pow_2 >> 32;                                          \
      next_pow_2++;                                                            \
      size_t new_size = next_pow_2;                                            \
                                                                               \
      if (insert_to_back) {                                                    \
        vec = (T##_vector_p)realloc(                                           \
            vec,                                                               \
            sizeof(T##_vector_t) + new_size * sizeof(C_TYPE));                 \
        vec->_max_size = new_size;                                             \
      } else {                                                                 \
        T##_vector_p new_vec = (T##_vector_p)malloc(                           \
            sizeof(T##_vector_t) + new_size * sizeof(C_TYPE));                 \
        size_t diff = (new_size - size_after_insert) / 2;                      \
        new_vec->_front = vec->_front + diff;                                  \
        new_vec->_back = new_vec->_front + size_after_insert;                  \
        new_vec->_max_size = new_size;                                         \
                                                                               \
        /* If we are inserting in the middle */                                \
        size_t current_idx = new_vec->_front;                                  \
        if (start > 0) {                                                       \
          /* Copy vec[front:front+start) to new[front:front+start)   */        \
          memcpy((void *)&new_vec->_storage[new_vec->_front],                  \
                 (const void *)&vec->_storage[vec->_front],                    \
                 sizeof(C_TYPE) * start);                                      \
          current_idx += start;                                                \
        }                                                                      \
                                                                               \
        /*Copy vec2[from:to) to new[front+start:front+start+to-from) */        \
        memcpy((void *)&new_vec->_storage[current_idx],                        \
               (const void *)&vec2->_storage[vec2->_front + from],             \
               sizeof(C_TYPE) * (to - from));                                  \
        current_idx += (to - from);                                            \
                                                                               \
        /* Copy vec[front+start:back) to */                                    \
        /* new[front+start+to-from:front+to-from+size) */                      \
        memcpy((void *)&new_vec->_storage[current_idx],                        \
               (const void *)&vec->_storage[vec->_front + start],              \
               sizeof(C_TYPE) * (vec_size - start));                           \
                                                                               \
        /* Free the old vector. */                                             \
        free(vec);                                                             \
                                                                               \
        return new_vec;                                                        \
      }                                                                        \
    }                                                                          \
                                                                               \
    size_t start_idx = vec->_front + start;                                    \
                                                                               \
    /* Move vec[front+start:back) to vec[front+start+ins_size:back+ins_size)   \
     */                                                                        \
    memmove((void *)&vec->_storage[start_idx + insert_size],                   \
            (const void *)&vec->_storage[start_idx],                           \
            sizeof(C_TYPE) * (vec_size - start));                              \
                                                                               \
    /* Copy vec2[from:to) to vec[start:start+to-from) */                       \
    memcpy((void *)&vec->_storage[start_idx],                                  \
           (const void *)&vec2->_storage[vec->_front],                         \
           sizeof(C_TYPE) * insert_size);                                      \
                                                                               \
    vec->_back += insert_size;                                                 \
                                                                               \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  static alwaysinline used T##_vector_p T##_vector__insert(                    \
      T##_vector_p vec,                                                        \
      size_t start,                                                            \
      T##_vector_p vec2) {                                                     \
    return T##_vector__insert_range(vec,                                       \
                                    start,                                     \
                                    vec2,                                      \
                                    0,                                         \
                                    vec2->_back - vec2->_front);               \
  }                                                                            \
                                                                               \
  static alwaysinline used T##_vector_p T##_vector__insert_element(            \
      T##_vector_p vec,                                                        \
      size_t start,                                                            \
      C_TYPE value) {                                                          \
    size_t vec_size = vec->_back - vec->_front;                                \
    size_t insert_size = 1;                                                    \
    size_t size_after_insert = vec_size + insert_size;                         \
                                                                               \
    bool insert_to_back = (start > 0);                                         \
                                                                               \
    if (size_after_insert > vec->_max_size) {                                  \
      /* grow the vector. */                                                   \
      size_t new_size = vec->_max_size << 1;                                   \
      if (insert_to_back) {                                                    \
        vec = (T##_vector_p)realloc(                                           \
            vec,                                                               \
            sizeof(T##_vector_t) + new_size * sizeof(C_TYPE));                 \
        vec->_max_size = new_size;                                             \
      } else {                                                                 \
        T##_vector_p new_vec = (T##_vector_p)malloc(                           \
            sizeof(T##_vector_t) + new_size * sizeof(C_TYPE));                 \
        size_t diff = (new_size - size_after_insert) / 2;                      \
        new_vec->_front = vec->_front + diff;                                  \
        new_vec->_back = new_vec->_front + size_after_insert;                  \
        new_vec->_max_size = new_size;                                         \
                                                                               \
        /* If we are inserting in the middle */                                \
        size_t current_idx = new_vec->_front;                                  \
        if (start > 0) {                                                       \
          /* Copy vec[front:front+start) to new[front:front+start) */          \
          memcpy((void *)&new_vec->_storage[new_vec->_front],                  \
                 (const void *)&vec->_storage[vec->_front],                    \
                 sizeof(C_TYPE) * start);                                      \
          current_idx += start;                                                \
        }                                                                      \
                                                                               \
        /* Write the element */                                                \
        new_vec->_storage[current_idx] = value;                                \
        current_idx += insert_size;                                            \
                                                                               \
        /* Copy vec[front+start:back) to */                                    \
        /* new[front+start+to-from:front+to-from+size) */                      \
        memcpy((void *)&new_vec->_storage[current_idx],                        \
               (const void *)&vec->_storage[vec->_front + start],              \
               sizeof(C_TYPE) * (vec_size - start));                           \
                                                                               \
        /* Free the old vector. */                                             \
        free(vec);                                                             \
                                                                               \
        return new_vec;                                                        \
      }                                                                        \
    }                                                                          \
                                                                               \
    size_t start_idx = vec->_front + start;                                    \
                                                                               \
    if (start_idx < vec->_back) {                                              \
      /* Move vec[front+start:back) to vec[front+s+ins_size:back+ins_size) */  \
      memmove((void *)&vec->_storage[start_idx + insert_size],                 \
              (const void *)&vec->_storage[start_idx],                         \
              sizeof(C_TYPE) * (vec_size - start));                            \
    }                                                                          \
                                                                               \
    /* Write the element to the index.*/                                       \
    vec->_storage[start_idx] = value;                                          \
    vec->_back += insert_size;                                                 \
                                                                               \
    return vec;                                                                \
  }                                                                            \
                                                                               \
  static alwaysinline used void T##_vector__swap(T##_vector_p vec,             \
                                                 size_t from,                  \
                                                 size_t to,                    \
                                                 size_t start) {               \
    size_t swap_size = to - from;                                              \
    from += vec->_front;                                                       \
    start += vec->_front;                                                      \
    /* TODO: vectorize this. */                                                \
    for (size_t idx = 0; idx < swap_size; idx++) {                             \
      C_TYPE tmp = vec->_storage[start + idx];                                 \
      vec->_storage[start + idx] = vec->_storage[from + idx];                  \
      vec->_storage[from + idx] = tmp;                                         \
    }                                                                          \
    return;                                                                    \
  }
#define HANDLE_INTEGER_TYPE(T, C_TYPE, BW, SIGNED)                             \
  INSTANTIATE_TYPED_VECTOR(T, C_TYPE)
#define HANDLE_PRIMITIVE_TYPE(T, C_TYPE, PREFIX)                               \
  INSTANTIATE_TYPED_VECTOR(T, C_TYPE)
#include "types.def"

#if defined(__cplusplus)
} // extern "C"
#endif
