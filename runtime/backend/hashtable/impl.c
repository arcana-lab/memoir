#include "hashtable.h"

// Generate the default hashtables.
#define HANDLE_INTEGER_TYPE(T, C_TYPE, BW, SIGNED)                             \
  INSTANTIATE_PTR_HASHTABLE(T, C_TYPE)                                         \
  INSTANTIATE_PRIMITIVE_HASHTABLE(u32, uint32_t, T, C_TYPE)
#define HANDLE_PRIMITIVE_TYPE(T, C_TYPE, PREFIX)                               \
  INSTANTIATE_PTR_HASHTABLE(T, C_TYPE)                                         \
  INSTANTIATE_PRIMITIVE_HASHTABLE(u32, uint32_t, T, C_TYPE)
#pragma pack(1)
typedef struct {
  char a[40];
} collection_t;
INSTANTIATE_PRIMITIVE_NESTING_HASHTABLE(u32, uint32_t, collection, collection_t)
#include "types.def"
