// EXPECTS:
//  CODE_0, the key type code
//  TYPE_0, the key C type
//  CODE_1, the value type code
//  TYPE_1, the value C type

#include <memoir/impl/utilities.h>

#include <memoir/impl/abseil_flat_hash_map/definition.hpp>

#define KEY_CODE CODE_0
#define KEY_TYPE TYPE_0
#define VAL_CODE CODE_1
#define VAL_TYPE TYPE_1

#define IMPL abseil_flat_hash_map
#define PREFIX CAT(KEY_CODE, CAT(_, CAT(VAL_CODE, CAT(_, IMPL))))

#define TYPE CAT(PREFIX, _t)
#define PTR CAT(PREFIX, _p)
typedef FlatHashMap<KEY_TYPE, VAL_TYPE> TYPE;
typedef TYPE *PTR;

#define ITER_TYPE CAT(PREFIX, _iter_t)
#define ITER_PTR CAT(PREFIX, _iter_p)
typedef TYPE::iterator ITER_TYPE;
typedef ITER_TYPE *ITER_PTR;

#undef IMPL
#undef KEY_CODE
#undef KEY_TYPE
#undef VAL_CODE
#undef VAL_TYPE
#undef PREFIX
#undef ITER_TYPE
#undef ITER_PTR
