// EXPECTS:
//  CODE_0, the element type code
//  TYPE_0, the element C type

#include <memoir/impl/utilities.h>

#include <memoir/impl/index_map/definition.hpp>

#define KEY_CODE CODE_0
#define KEY_TYPE TYPE_0
#define VAL_CODE CODE_1
#define VAL_TYPE TYPE_1

#define IMPL index_map
#define PREFIX CAT(KEY_CODE, CAT(_, CAT(VAL_CODE, CAT(_, IMPL))))

#define TYPE CAT(PREFIX, _t)
#define PTR CAT(PREFIX, _p)
typedef IndexMap<KEY_TYPE, VAL_TYPE> TYPE;
typedef TYPE *PTR;

#define ENC_TYPE CAT(PREFIX, _enc_t)
#define ENC_PTR CAT(PREFIX, _enc_p)
typedef TYPE::Encoder ENC_TYPE;
typedef ENC_TYPE *ENC_PTR;

#define DEC_TYPE CAT(PREFIX, _dec_t)
#define DEC_PTR CAT(PREFIX, _dec_p)
typedef TYPE::Decoder DEC_TYPE;
typedef DEC_TYPE *DEC_PTR;

#define ITER_TYPE CAT(PREFIX, _iter_t)
#define ITER_PTR CAT(PREFIX, _iter_p)
typedef TYPE::iterator ITER_TYPE;
typedef ITER_TYPE *ITER_PTR;

#undef KEY_CODE
#undef KEY_TYPE
#undef VAL_CODE
#undef VAL_TYPE
#undef PREFIX
#undef TYPE
#undef PTR
#undef ENC_TYPE
#undef ENC_PTR
#undef ITER_TYPE
#undef ITER_PTR
