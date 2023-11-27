#include "vector.h"

#define HANDLE_INTEGER_TYPE(T, C_TYPE, BW, SIGNED)                             \
  INSTANTIATE_TYPED_VECTOR(T, C_TYPE)
#define HANDLE_PRIMITIVE_TYPE(T, C_TYPE, PREFIX)                               \
  INSTANTIATE_TYPED_VECTOR(T, C_TYPE)
#include "types.def"

typedef struct memoir_basket {
  char a[32];
} memoir_basket_t;
INSTANTIATE_TYPED_NESTING_VECTOR(struct, memoir_basket_t);
