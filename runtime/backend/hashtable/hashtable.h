// Simple hash table implemented in C.
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined(INITIAL_CAPACITY)
#  define INITIAL_CAPACITY (1 << 20) // must not be zero
/* #  define INITIAL_CAPACITY 150000000 // deepsjeng_s BIG_MEMORY */
#endif

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

static alwaysinline used uint32_t u32_hashtable__hash_key(uint32_t key) {
  key = ((key >> 16) ^ key) * 0x45d9f3b;
  key = ((key >> 16) ^ key) * 0x45d9f3b;
  key = (key >> 16) ^ key;
  return key;
}

// NOTE: this implementation does not have pointer stability. To use this
// implementation, the non-necessity of pointer stability must be proved.
#define INSTANTIATE_PRIMITIVE_NESTING_HASHTABLE(K, C_KEY, V, C_VALUE)          \
  typedef struct K##_##V##_hashtable_entry {                                   \
    bool _exists;                                                              \
    C_KEY _key;                                                                \
    C_VALUE _value;                                                            \
  } K##_##V##_hashtable_entry_t;                                               \
  typedef K##_##V##_hashtable_entry_t *K##_##V##_hashtable_entry_p;            \
  typedef struct K##_##V##_hashtable {                                         \
    size_t _capacity;                                                          \
    size_t _length;                                                            \
    K##_##V##_hashtable_entry_t *_entries;                                     \
  } K##_##V##_hashtable_t;                                                     \
  typedef K##_##V##_hashtable_t *K##_##V##_hashtable_p;                        \
  typedef struct K##_##V##_hashtable_keys {                                    \
    K##_##V##_hashtable_p table;                                               \
  } K##_##V##_hashtable_keys_t;                                                \
  typedef K##_##V##_hashtable_keys_t *K##_##V##_hashtable_keys_p;              \
                                                                               \
  static alwaysinline used                                                     \
      K##_##V##_hashtable_p K##_##V##_hashtable__allocate(void) {              \
    K##_##V##_hashtable_p table =                                              \
        (K##_##V##_hashtable_p)malloc(sizeof(K##_##V##_hashtable_t));          \
    if (table == NULL) {                                                       \
      exit(1);                                                                 \
    }                                                                          \
    table->_entries =                                                          \
        calloc(INITIAL_CAPACITY, sizeof(K##_##V##_hashtable_entry_t));         \
    if (table->_entries == NULL) {                                             \
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
    free(table->_entries);                                                     \
    free(table);                                                               \
  }                                                                            \
                                                                               \
  static alwaysinline used bool K##_##V##_hashtable__has(                      \
      K##_##V##_hashtable_p table,                                             \
      C_KEY key) {                                                             \
    uint32_t hash = K##_hashtable__hash_key(key);                              \
    size_t index = (size_t)(hash % (uint32_t)table->_capacity);                \
    size_t begin_index = index;                                                \
                                                                               \
    /* Loop 'til we find an empty entry. */                                    \
    while (table->_entries[index]._exists) {                                   \
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
  static alwaysinline used K##_##V##_hashtable_p K##_##V##_hashtable__expand(  \
      K##_##V##_hashtable_p);                                                  \
                                                                               \
  /* Get key-value pair */                                                     \
  static alwaysinline used C_VALUE *K##_##V##_hashtable__get(                  \
      K##_##V##_hashtable_p table,                                             \
      C_KEY key) {                                                             \
    if ((table->_length + 1) > (table->_capacity / 2)) {                       \
      table = K##_##V##_hashtable__expand(table);                              \
    }                                                                          \
                                                                               \
    uint32_t hash = K##_hashtable__hash_key(key);                              \
    size_t index = (size_t)(hash % (uint32_t)table->_capacity);                \
                                                                               \
    /* Loop 'til we find an empty entry. */                                    \
    while (table->_entries[index]._exists) {                                   \
      if (key == table->_entries[index]._key) {                                \
        /* Found key. */                                                       \
        return &table->_entries[index]._value;                                 \
      }                                                                        \
      /* Do a linear probe. */                                                 \
      index++;                                                                 \
      if (index >= table->_capacity) {                                         \
        index = 0;                                                             \
      }                                                                        \
    }                                                                          \
                                                                               \
    /* Write the key-value pair. */                                            \
    table->_entries[index]._exists = true;                                     \
    table->_entries[index]._key = key;                                         \
    table->_length++;                                                          \
                                                                               \
    return &table->_entries[index]._value;                                     \
  }                                                                            \
                                                                               \
  static alwaysinline used K##_##V##_hashtable_p K##_##V##_hashtable__remove(  \
      K##_##V##_hashtable_p table,                                             \
      C_KEY key) {                                                             \
    uint32_t hash = K##_hashtable__hash_key(key);                              \
    size_t index = (size_t)(hash % (uint32_t)table->_capacity);                \
    size_t begin_index = index;                                                \
                                                                               \
    /* Loop 'til we find the entry. */                                         \
    while (table->_entries[index]._exists) {                                   \
      if (key == table->_entries[index]._key) {                                \
        /* Found key. */                                                       \
        table->_entries[index]._exists = false;                                \
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
    size_t new_capacity = (table->_capacity) + (table->_capacity >> 1);        \
    if (new_capacity < table->_capacity) { /* OVERFLOW! */                     \
      exit(2);                                                                 \
    }                                                                          \
                                                                               \
    /* Allocate entries for the new capacity */                                \
    K##_##V##_hashtable_entry_p new_entries = (K##_##V##_hashtable_entry_p)    \
        calloc(new_capacity, sizeof(K##_##V##_hashtable_entry_t));             \
    if (new_entries == NULL) { /* OoM! */                                      \
      exit(1);                                                                 \
    }                                                                          \
                                                                               \
    /* Iterate entries, move all non-empty ones to new table's entries. */     \
    for (size_t i = 0; i < table->_capacity; i++) {                            \
      if (table->_entries[i]._exists) {                                        \
        /* Hash the key into the new table */                                  \
        uint32_t hash = K##_hashtable__hash_key(table->_entries[i]._key);      \
        size_t new_i = (size_t)(hash % (uint32_t)new_capacity);                \
        /* Linear probe until we find an empty slot */                         \
        while (new_entries[new_i]._exists) {                                   \
          new_i++;                                                             \
        }                                                                      \
        /* memcpy the old value into the new table */                          \
        new_entries[new_i]._exists = true;                                     \
        new_entries[new_i]._key = table->_entries[i]._key;                     \
        memcpy(&new_entries[new_i]._value,                                     \
               &table->_entries[i]._value,                                     \
               sizeof(C_VALUE));                                               \
      }                                                                        \
    }                                                                          \
    table->_capacity = new_capacity;                                           \
                                                                               \
    /* Free the old table */                                                   \
    free(table->_entries);                                                     \
    table->_entries = new_entries;                                             \
                                                                               \
    return table;                                                              \
  }                                                                            \
                                                                               \
  static alwaysinline used size_t K##_##V##_hashtable__size(                   \
      K##_##V##_hashtable_p table) {                                           \
    return table->_length;                                                     \
  }

#define INSTANTIATE_PRIMITIVE_HASHTABLE(K, C_KEY, V, C_VALUE)                  \
  typedef struct K##_##V##_hashtable_entry {                                   \
    bool _exists;                                                              \
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
  static alwaysinline used bool K##_##V##_hashtable__has(                      \
      K##_##V##_hashtable_p table,                                             \
      C_KEY key) {                                                             \
    uint64_t hash = K##_hashtable__hash_key(key);                              \
    size_t index = (size_t)(hash & (uint64_t)(table->_capacity - 1));          \
    size_t begin_index = index;                                                \
                                                                               \
    /* Loop 'til we find an empty entry. */                                    \
    while (table->_entries[index]._exists) {                                   \
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
    hash = K##_hashtable__hash_key(key);                                       \
    index = (size_t)(hash & (uint64_t)(table->_capacity - 1));                 \
    begin_index = index;                                                       \
                                                                               \
    /* Loop 'til we find an empty entry. */                                    \
    while (table->_entries[index]._exists) {                                   \
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
    table->_entries[index]._exists = true;                                     \
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
    uint64_t hash = K##_hashtable__hash_key(key);                              \
    size_t index = (size_t)(hash & (uint64_t)(table->_capacity - 1));          \
    size_t begin_index = index;                                                \
                                                                               \
    /* Loop 'til we find the entry. */                                         \
    while (table->_entries[index]._exists) {                                   \
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
    uint64_t hash = K##_hashtable__hash_key(key);                              \
    size_t index = (size_t)(hash & (uint64_t)(table->_capacity - 1));          \
    size_t begin_index = index;                                                \
                                                                               \
    /* Loop 'til we find the entry. */                                         \
    while (table->_entries[index]._exists) {                                   \
      if (key == table->_entries[index]._key) {                                \
        /* Found key. */                                                       \
        table->_entries[index]._exists = false;                                \
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
      if (table->_entries[i]._exists) {                                        \
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

static alwaysinline used uint64_t ptr_hashtable__hash_key(void *key) {
  uint64_t hash = FNV_OFFSET;
  hash *= (uint64_t)key;
  hash ^= (uint64_t)key >> 15;
  return hash;
}

#define INSTANTIATE_PTR_HASHTABLE(V, C_VALUE)                                  \
  typedef struct ptr_##V##_hashtable_entry {                                   \
    void *_key;                                                                \
    C_VALUE _value;                                                            \
  } ptr_##V##_hashtable_entry_t;                                               \
  typedef ptr_##V##_hashtable_entry_t *ptr_##V##_hashtable_entry_p;            \
  typedef struct ptr_##V##_hashtable {                                         \
    size_t _capacity;                                                          \
    size_t _length;                                                            \
    ptr_##V##_hashtable_entry_t _entries[];                                    \
  } ptr_##V##_hashtable_t;                                                     \
  typedef ptr_##V##_hashtable_t *ptr_##V##_hashtable_p;                        \
  typedef struct ptr_##V##_hashtable_keys {                                    \
    ptr_##V##_hashtable_p table;                                               \
  } ptr_##V##_hashtable_keys_t;                                                \
  typedef ptr_##V##_hashtable_keys_t *ptr_##V##_hashtable_keys_p;              \
                                                                               \
  static alwaysinline used                                                     \
      ptr_##V##_hashtable_p ptr_##V##_hashtable__allocate(void) {              \
    ptr_##V##_hashtable_p table = (ptr_##V##_hashtable_p)malloc(               \
        sizeof(ptr_##V##_hashtable_t)                                          \
        + INITIAL_CAPACITY * sizeof(ptr_##V##_hashtable_entry_t));             \
    memset(table->_entries,                                                    \
           0,                                                                  \
           INITIAL_CAPACITY * sizeof(ptr_##V##_hashtable_entry_t));            \
    if (table == NULL) {                                                       \
      exit(1);                                                                 \
    }                                                                          \
    table->_capacity = INITIAL_CAPACITY;                                       \
    table->_length = 0;                                                        \
                                                                               \
    return table;                                                              \
  }                                                                            \
                                                                               \
  static alwaysinline used void ptr_##V##_hashtable__free(                     \
      ptr_##V##_hashtable_p table) {                                           \
    free(table);                                                               \
  }                                                                            \
                                                                               \
  static alwaysinline used bool ptr_##V##_hashtable__has(                      \
      ptr_##V##_hashtable_p table,                                             \
      void *key) {                                                             \
    uint64_t hash = ptr_hashtable__hash_key(key);                              \
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
  static alwaysinline used void ptr_##V##_hashtable__set(                      \
      ptr_##V##_hashtable_p table,                                             \
      void *key,                                                               \
      C_VALUE value) {                                                         \
    uint64_t hash;                                                             \
    size_t index, begin_index;                                                 \
    hash = ptr_hashtable__hash_key(key);                                       \
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
  static alwaysinline used ptr_##V##_hashtable_p ptr_##V##_hashtable__expand(  \
      ptr_##V##_hashtable_p);                                                  \
  static alwaysinline used ptr_##V##_hashtable_p ptr_##V##_hashtable__write(   \
      ptr_##V##_hashtable_p table,                                             \
      void *key,                                                               \
      C_VALUE value) {                                                         \
    /* Resize if we would be more than half full. */                           \
    if (table->_length >= table->_capacity / 2) {                              \
      table = ptr_##V##_hashtable__expand(table);                              \
    }                                                                          \
                                                                               \
    ptr_##V##_hashtable__set(table, key, value);                               \
                                                                               \
    return table;                                                              \
  }                                                                            \
                                                                               \
  static alwaysinline used C_VALUE ptr_##V##_hashtable__read(                  \
      ptr_##V##_hashtable_p table,                                             \
      void *key) {                                                             \
    uint64_t hash = ptr_hashtable__hash_key(key);                              \
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
  static alwaysinline used ptr_##V##_hashtable_p ptr_##V##_hashtable__remove(  \
      ptr_##V##_hashtable_p table,                                             \
      void *key) {                                                             \
    uint64_t hash = ptr_hashtable__hash_key(key);                              \
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
  static alwaysinline used ptr_##V##_hashtable_p ptr_##V##_hashtable__expand(  \
      ptr_##V##_hashtable_p table) {                                           \
    size_t new_capacity = table->_capacity * 2;                                \
    if (new_capacity < table->_capacity) { /* OVERFLOW! */                     \
      exit(2);                                                                 \
    }                                                                          \
                                                                               \
    ptr_##V##_hashtable_p new_table = (ptr_##V##_hashtable_p)malloc(           \
        sizeof(ptr_##V##_hashtable_t)                                          \
        + new_capacity * sizeof(ptr_##V##_hashtable_entry_t));                 \
    memset(new_table->_entries,                                                \
           0,                                                                  \
           new_capacity * sizeof(ptr_##V##_hashtable_entry_t));                \
    if (new_table == NULL) { /* OoM! */                                        \
      exit(1);                                                                 \
    }                                                                          \
    new_table->_capacity = new_capacity;                                       \
                                                                               \
    /* Iterate entries, move all non-empty ones to new table's entries. */     \
    for (size_t i = 0; i < table->_capacity; i++) {                            \
      if (table->_entries[i]._key != NULL) {                                   \
        ptr_##V##_hashtable__set(new_table,                                    \
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
  static alwaysinline used size_t ptr_##V##_hashtable__size(                   \
      ptr_##V##_hashtable_p table) {                                           \
    return table->_length;                                                     \
  }

#if defined(__cplusplus)
} // extern "C"
#endif
