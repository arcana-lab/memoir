#include "stl_unordered_map.hh"

// Generate the default hashtables.
// #define HANDLE_INTEGER_TYPE(T, C_TYPE, BW, SIGNED) \
//   INSTANTIATE_PTR_ASSOC(T, C_TYPE) \ INSTANTIATE_PRIMITIVE_ASSOC(u32,
//   uint32_t, T, C_TYPE)
// #define HANDLE_PRIMITIVE_TYPE(T, C_TYPE, PREFIX) \
//   INSTANTIATE_PTR_ASSOC(T, C_TYPE) \ INSTANTIATE_PRIMITIVE_ASSOC(u32,
//   uint32_t, T, C_TYPE)
/* #include "types.def" */

#pragma pack(1)
typedef struct {
  char a[40];
} collection_t;
INSTANTIATE_STL_UNORDERED_MAP(u32, uint32_t, collection, collection_t)
