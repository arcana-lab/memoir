// Simple hash table implemented in C.
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 16 // must not be zero

#define alwaysinline __attribute__((always_inline)) inline
#define used __attribute__((used))

#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME 1099511628211UL

#if defined(STORE_LENGTH)
#else
#endif

#if defined(__cplusplus)
extern "C" {
#endif

// Only works for pointers
#define INSTANTIATE_TYPED_HASHTABLE(K, C_KEY, V, C_VALUE)                      \
  typedef struct K##_##V##_hashtable_entry {                                   \
    C_KEY _key;                                                                \
    C_VALUE _value;                                                            \
  } K##_##V##_hashtable_entry_t;                                               \
  typedef K##_##V##_hashtable_entry_t *K##_##V##_hashtable_entry_p;            \
  typedef struct K##_##V##_hashtable {                                         \
    size_t _capacity;                                                          \
    size_t _length;                                                            \
    K##_##V##_hashtable_entry_t _entries[];                                    \
  } K##_##V##_hashtable_t;                                                     \
  typedef K##_##V##_hashtable_t *K##_##V##_hashtable_p;                        \
  typedef struct K##_##V##_hashtable_keys {                                    \
    K##_##V##_hashtable_p table;                                               \
  } K##_##V##_hashtable_keys_t;                                                \
  typedef K##_##V##_hashtable_keys_t *K##_##V##_hashtable_keys_p;              \
                                                                               \
  static alwaysinline used                                                     \
      K##_##V##_hashtable_p K##_##V##_hashtable__allocate(void) {              \
    K##_##V##_hashtable_p table = (K##_##V##_hashtable_p)malloc(               \
        sizeof(K##_##V##_hashtable_t)                                          \
        + INITIAL_CAPACITY * sizeof(K##_##V##_hashtable_entry_t));             \
    memset(table->_entries,                                                    \
           0,                                                                  \
           INITIAL_CAPACITY * sizeof(K##_##V##_hashtable_entry_t));            \
    if (table == NULL) {                                                       \
      exit(1);                                                                 \
    }                                                                          \
    table->_capacity = INITIAL_CAPACITY;                                       \
    table->_length = 0;                                                        \
                                                                               \
    return table;                                                              \
  }                                                                            \
                                                                               \
  static alwaysinline used void K##_##V##_hashtable__free(                     \
      K##_##V##_hashtable_p table) {                                           \
    free(table);                                                               \
  }                                                                            \
                                                                               \
  static alwaysinline used uint64_t K##_##V##_hashtable__hash_key(void *key) { \
    uint64_t hash = FNV_OFFSET;                                                \
    hash *= (uint64_t)key;                                                     \
    hash ^= (uint64_t)key >> 15;                                               \
    return hash;                                                               \
  }                                                                            \
                                                                               \
  static alwaysinline used bool K##_##V##_hashtable__has(                      \
      K##_##V##_hashtable_p table,                                             \
      C_KEY key) {                                                             \
    uint64_t hash = K##_##V##_hashtable__hash_key(key);                        \
    size_t index = (size_t)(hash & (uint64_t)(table->_capacity - 1));          \
    size_t begin_index = index;                                                \
                                                                               \
    /* Loop 'til we find an empty entry. */                                    \
    while (table->_entries[index]._key != NULL) {                              \
      if (key == table->_entries[index]._key) {                                \
        /* Found key */                                                        \
        return true;                                                           \
      }                                                                        \
      /* Do a linear probe */                                                  \
      index++;                                                                 \
      if (index == begin_index) {                                              \
        /* Couldn't find */                                                    \
        return false;                                                          \
      } else if (index >= table->_capacity) {                                  \
        index = 0;                                                             \
      }                                                                        \
    }                                                                          \
                                                                               \
    return false;                                                              \
  }                                                                            \
                                                                               \
  /* Set key-value pair, will not expand table */                              \
  static alwaysinline used void K##_##V##_hashtable__set(                      \
      K##_##V##_hashtable_p table,                                             \
      C_KEY key,                                                               \
      C_VALUE value) {                                                         \
    uint64_t hash;                                                             \
    size_t index, begin_index;                                                 \
    hash = K##_##V##_hashtable__hash_key(key);                                 \
    index = (size_t)(hash & (uint64_t)(table->_capacity - 1));                 \
    begin_index = index;                                                       \
                                                                               \
    /* Loop 'til we find an empty entry. */                                    \
    while (table->_entries[index]._key != NULL) {                              \
      if (key == table->_entries[index]._key) {                                \
        /* Found key. */                                                       \
        table->_entries[index]._value = value;                                 \
        return;                                                                \
      }                                                                        \
      /* Do a linear probe. */                                                 \
      index++;                                                                 \
      if (index >= table->_capacity) {                                         \
        index = 0;                                                             \
      }                                                                        \
    }                                                                          \
                                                                               \
    /* Write they key-value pair. */                                           \
    table->_entries[index]._key = key;                                         \
    table->_entries[index]._value = value;                                     \
    table->_length++;                                                          \
                                                                               \
    return;                                                                    \
  }                                                                            \
  static alwaysinline used K##_##V##_hashtable_p K##_##V##_hashtable__expand(  \
      K##_##V##_hashtable_p);                                                  \
  static alwaysinline used K##_##V##_hashtable_p K##_##V##_hashtable__write(   \
      K##_##V##_hashtable_p table,                                             \
      C_KEY key,                                                               \
      C_VALUE value) {                                                         \
    /* Resize if we would be more than half full. */                           \
    if (table->_length >= table->_capacity / 2) {                              \
      table = K##_##V##_hashtable__expand(table);                              \
    }                                                                          \
                                                                               \
    K##_##V##_hashtable__set(table, key, value);                               \
                                                                               \
    return table;                                                              \
  }                                                                            \
                                                                               \
  static alwaysinline used C_VALUE K##_##V##_hashtable__read(                  \
      K##_##V##_hashtable_p table,                                             \
      C_KEY key) {                                                             \
    uint64_t hash = K##_##V##_hashtable__hash_key(key);                        \
    size_t index = (size_t)(hash & (uint64_t)(table->_capacity - 1));          \
    size_t begin_index = index;                                                \
                                                                               \
    /* Loop 'til we find the entry. */                                         \
    while (table->_entries[index]._key != NULL) {                              \
      if (key == table->_entries[index]._key) {                                \
        /* Found key. */                                                       \
        return table->_entries[index]._value;                                  \
      }                                                                        \
      /* Do a linear probe. */                                                 \
      index++;                                                                 \
      if (index == begin_index) {                                              \
        return (C_VALUE)0;                                                     \
      } else if (index >= table->_capacity) {                                  \
        index = 0;                                                             \
      }                                                                        \
    }                                                                          \
                                                                               \
    return (C_VALUE)0;                                                         \
  }                                                                            \
                                                                               \
  static alwaysinline used K##_##V##_hashtable_p K##_##V##_hashtable__remove(  \
      K##_##V##_hashtable_p table,                                             \
      C_KEY key) {                                                             \
    uint64_t hash = K##_##V##_hashtable__hash_key(key);                        \
    size_t index = (size_t)(hash & (uint64_t)(table->_capacity - 1));          \
    size_t begin_index = index;                                                \
                                                                               \
    /* Loop 'til we find the entry. */                                         \
    while (table->_entries[index]._key != NULL) {                              \
      if (key == table->_entries[index]._key) {                                \
        /* Found key. */                                                       \
        table->_entries[index]._key = NULL;                                    \
        table->_length--;                                                      \
        return table;                                                          \
      }                                                                        \
      /* Do a linear probe. */                                                 \
      index++;                                                                 \
      if (index == begin_index) {                                              \
        return table;                                                          \
      } else if (index >= table->_capacity) {                                  \
        index = 0;                                                             \
      }                                                                        \
    }                                                                          \
                                                                               \
    return table;                                                              \
  }                                                                            \
                                                                               \
  /* Expand hash table to twice its current size. */                           \
  /* Return pointer to new table on success, NULL if out of memory. */         \
  static alwaysinline used K##_##V##_hashtable_p K##_##V##_hashtable__expand(  \
      K##_##V##_hashtable_p table) {                                           \
    size_t new_capacity = table->_capacity * 2;                                \
    if (new_capacity < table->_capacity) { /* OVERFLOW! */                     \
      exit(2);                                                                 \
    }                                                                          \
                                                                               \
    K##_##V##_hashtable_p new_table = (K##_##V##_hashtable_p)malloc(           \
        sizeof(K##_##V##_hashtable_t)                                          \
        + new_capacity * sizeof(K##_##V##_hashtable_entry_t));                 \
    memset(new_table->_entries,                                                \
           0,                                                                  \
           new_capacity * sizeof(K##_##V##_hashtable_entry_t));                \
    if (new_table == NULL) { /* OoM! */                                        \
      exit(1);                                                                 \
    }                                                                          \
    new_table->_capacity = new_capacity;                                       \
                                                                               \
    /* Iterate entries, move all non-empty ones to new table's entries. */     \
    for (size_t i = 0; i < table->_capacity; i++) {                            \
      if (table->_entries[i]._key != NULL) {                                   \
        K##_##V##_hashtable__set(new_table,                                    \
                                 table->_entries[i]._key,                      \
                                 table->_entries[i]._value);                   \
      }                                                                        \
    }                                                                          \
                                                                               \
    new_table->_length = table->_length;                                       \
                                                                               \
    /* Free the old table */                                                   \
    free(table);                                                               \
                                                                               \
    return new_table;                                                          \
  }                                                                            \
                                                                               \
  static alwaysinline used size_t K##_##V##_hashtable__size(                   \
      K##_##V##_hashtable_p table) {                                           \
    return table->_length;                                                     \
  }

#define HANDLE_INTEGER_TYPE(T, C_TYPE, BW, SIGNED)                             \
  INSTANTIATE_TYPED_HASHTABLE(ptr, void *, T, C_TYPE)
#define HANDLE_PRIMITIVE_TYPE(T, C_TYPE, PREFIX)                               \
  INSTANTIATE_TYPED_HASHTABLE(ptr, void *, T, C_TYPE)
#include "types.def"

#if defined(__cplusplus)
} // extern "C"
#endif
