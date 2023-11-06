#include "stl_unordered_map.h"

// Generate the default hashtables.
#define HANDLE_INTEGER_TYPE(T, C_TYPE, BW, SIGNED)                             \
  INSTANTIATE_stl_unordered_map(ptr, void *, T, C_TYPE)                        \
      INSTANTIATE_stl_unordered_map(struct_ref, void *, T, C_TYPE)
#define HANDLE_PRIMITIVE_TYPE(T, C_TYPE, PREFIX)                               \
  INSTANTIATE_stl_unordered_map(ptr, void *, T, C_TYPE)                        \
      INSTANTIATE_stl_unordered_map(struct_ref, void *, T, C_TYPE)
#include "types.def"

#pragma pack(1)
typedef struct {
  char a[40];
} collection_t;
INSTANTIATE_stl_unordered_map(u32, uint32_t, collection, collection_t)
