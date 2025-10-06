// EXPECTS:
//  CODE_0, the element type code
//  TYPE_0, the element C type

#include "memoir/impl/utilities.h"

#include "memoir/impl/stl_vector/definition.hpp"

#define IMPL stl_vector
#define PREFIX CAT(CODE_0, CAT(_, IMPL))

#define TYPE CAT(PREFIX, _t)
#define PTR CAT(PREFIX, _p)
typedef Vector<TYPE_0> TYPE;
typedef TYPE *PTR;

#define ITER_TYPE CAT(PREFIX, _iter_t)
#define ITER_PTR CAT(PREFIX, _iter_p)
typedef Vector<TYPE_0>::iterator ITER_TYPE;
typedef ITER_TYPE *ITER_PTR;

#define RITER_TYPE CAT(PREFIX, _riter_t)
#define RITER_PTR CAT(PREFIX, _riter_p)
typedef Vector<TYPE_0>::reverse_iterator RITER_TYPE;
typedef RITER_TYPE *RITER_PTR;

#undef IMPL
#undef PREFIX
#undef TYPE
#undef PTR
#undef ITER_TYPE
#undef ITER_PTR
#undef RITER_TYPE
#undef RITER_PTR
