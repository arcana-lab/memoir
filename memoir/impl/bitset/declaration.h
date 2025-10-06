// EXPECTS:
//  CODE_0, the element type code
//  TYPE_0, the element C type

#include <memoir/impl/utilities.h>

#include <memoir/impl/bitset/definition.hpp>

#define KEY_CODE CODE_0
#define KEY_TYPE TYPE_0

#define SIZE_TYPE size_t

#define IMPL bitset
#define PREFIX CAT(KEY_CODE, CAT(_, IMPL))

#define TYPE CAT(PREFIX, _t)
#define PTR CAT(PREFIX, _p)
typedef BitSet<KEY_TYPE> TYPE;
typedef TYPE *PTR;

#define ITER_TYPE CAT(PREFIX, _iter_t)
#define ITER_PTR CAT(PREFIX, _iter_p)
typedef TYPE::iterator ITER_TYPE;
typedef ITER_TYPE *ITER_PTR;

#undef KEY_CODE
#undef KEY_TYPE
#undef SIZE_TYPE
#undef IMPL
#undef PREFIX
#undef ITER_TYPE
#undef ITER_PTR
