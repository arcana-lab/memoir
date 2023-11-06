#include "stl_vector.h"

INSTANTIATE_stl_vector(ptr, void *)
INSTANTIATE_stl_vector(u64, uint64_t)

#pragma pack(1)
    typedef struct {
  char a[32];
} memoir_bucket_t;
INSTANTIATE_stl_vector(struct, memoir_bucket_t)
