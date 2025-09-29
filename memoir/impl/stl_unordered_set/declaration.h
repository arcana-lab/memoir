// EXPECTS:
//  CODE_0, the element type code
//  TYPE_0, the element C type

#include <backend/utilities.h>

#include <backend/stl_unordered_set/definition.hpp>

#define KEY_CODE CODE_0
#define KEY_TYPE TYPE_0

#define IMPL stl_unordered_set
#define PREFIX CAT(KEY_CODE, CAT(_, IMPL))

#define TYPE CAT(PREFIX, _t)
#define PTR CAT(PREFIX, _p)
typedef UnorderedSet<KEY_TYPE> TYPE;
typedef TYPE *PTR;

#define ITER_TYPE CAT(PREFIX, _iter_t)
#define ITER_PTR CAT(PREFIX, _iter_p)
typedef TYPE::iterator ITER_TYPE;
typedef ITER_TYPE *ITER_PTR;

#undef KEY_CODE
#undef KEY_TYPE
#undef IMPL
#undef PREFIX
#undef TYPE
#undef PTR
#undef ITER_TYPE
#undef ITER_PTR
