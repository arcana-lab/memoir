// EXPECTS:
//  CODE_0, the element type code
//  TYPE_0, the element C type

#include <memoir/impl/utilities.h>

#include <memoir/impl/boost_flat_set/definition.hpp>

#define KEY_CODE CODE_0
#define KEY_TYPE TYPE_0

#define PREFIX CAT(KEY_CODE, _boost_flat_set)

#define TYPE CAT(PREFIX, _t)
#define PTR CAT(PREFIX, _p)
typedef FlatSet<KEY_TYPE> TYPE;
typedef TYPE *PTR;

#define ITER_TYPE CAT(PREFIX, _iter_t)
#define ITER_PTR CAT(PREFIX, _iter_p)
typedef TYPE::iterator ITER_TYPE;
typedef ITER_TYPE *ITER_PTR;

#undef KEY_CODE
#undef KEY_TYPE
#undef PREFIX
#undef ITER_TYPE
#undef ITER_PTR
