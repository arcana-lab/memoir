// EXPECTS:
//  CODE_0, the element type code
//  TYPE_0, the element C type

#include <backend/utilities.h>

#include <backend/index_set/definition.hpp>

#define KEY_CODE CODE_0
#define KEY_TYPE TYPE_0
#define VAL_CODE CODE_1
#define VAL_TYPE TYPE_1

#define IMPL index_set
#define PREFIX CAT(KEY_CODE, CAT(_, CAT(VAL_CODE, CAT(_, IMPL))))

#define TYPE CAT(PREFIX, _t)
#define PTR CAT(PREFIX, _p)
typedef IndexSet<KEY_TYPE, VAL_TYPE> TYPE;
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

typedef size_t SIZE_TYPE;

#define OP(op) CAT(CAT(PREFIX, __), op)

CNAME ALWAYS_INLINE USED PTR OP(allocate)() {
  return new TYPE();
}

CNAME ALWAYS_INLINE USED ENC_PTR OP(allocate_encoder)() {
  return new ENC_TYPE();
}

CNAME ALWAYS_INLINE USED DEC_PTR OP(allocate_decoder)() {
  return new DEC_TYPE();
}

CNAME ALWAYS_INLINE USED void OP(set_encoder)(PTR set, ENC_PTR enc) {
  return set->encoder(enc);
}

CNAME ALWAYS_INLINE USED void OP(set_decoder)(PTR set, DEC_PTR dec) {
  return set->decoder(dec);
}

CNAME ALWAYS_INLINE USED ENC_PTR OP(get_encoder)(PTR set) {
  return set->encoder();
}

CNAME ALWAYS_INLINE USED DEC_PTR OP(get_decoder)(PTR set) {
  return set->decoder();
}

CNAME ALWAYS_INLINE USED void OP(free)(PTR set) {
  delete set;
}

CNAME ALWAYS_INLINE USED PTR OP(copy)(PTR set) {
  return set->copy();
}

CNAME ALWAYS_INLINE USED PTR OP(remove_encoded)(PTR set, SIZE_TYPE i) {
  set->remove_encoded(i);
  return set;
}

CNAME ALWAYS_INLINE USED PTR OP(remove)(PTR set, KEY_TYPE key) {
  set->remove(key);
  return set;
}

CNAME ALWAYS_INLINE USED PTR OP(insert_encoded)(PTR set, SIZE_TYPE i) {
  set->insert_encoded(i);
  return set;
}

CNAME ALWAYS_INLINE USED PTR OP(insert)(PTR set, KEY_TYPE key) {
  set->insert(key);
  return set;
}

CNAME ALWAYS_INLINE USED bool OP(has_encoded)(PTR set, SIZE_TYPE i) {
  return set->has_encoded(i);
}

CNAME ALWAYS_INLINE USED bool OP(has)(PTR set, KEY_TYPE key) {
  return set->has(key);
}

CNAME ALWAYS_INLINE USED size_t OP(size)(PTR set) {
  return set->size();
}

CNAME ALWAYS_INLINE USED PTR OP(clear)(PTR set) {
  set->clear();
  return set;
}

CNAME ALWAYS_INLINE USED void OP(begin)(ITER_PTR iter, PTR set) {
  set->begin(iter);
}

CNAME ALWAYS_INLINE USED bool OP(next)(ITER_PTR iter) {
  return iter->next();
}

#undef KEY_CODE
#undef KEY_TYPE
#undef VAL_CODE
#undef VAL_TYPE
#undef PREFIX
#undef TYPE
#undef PTR
#undef ENC_TYPE
#undef ENC_PTR
#undef DEC_TYPE
#undef DEC_PTR
#undef ITER_TYPE
#undef ITER_PTR
