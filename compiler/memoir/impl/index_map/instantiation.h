// EXPECTS:
//  CODE_0, the element type code
//  TYPE_0, the element C type

#include <backend/utilities.h>

#include <backend/index_map/definition.hpp>

#define KEY_CODE CODE_0
#define KEY_TYPE TYPE_0
#define VAL_CODE CODE_1
#define VAL_TYPE TYPE_1

#define IMPL index_map
#define PREFIX CAT(KEY_CODE, CAT(_, CAT(VAL_CODE, CAT(_, IMPL))))

#define TYPE CAT(PREFIX, _t)
#define PTR CAT(PREFIX, _p)

#define ENC_TYPE CAT(PREFIX, _enc_t)
#define ENC_PTR CAT(PREFIX, _enc_p)

#define DEC_TYPE CAT(PREFIX, _dec_t)
#define DEC_PTR CAT(PREFIX, _dec_p)

#define ITER_TYPE CAT(PREFIX, _iter_t)
#define ITER_PTR CAT(PREFIX, _iter_p)

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

CNAME ALWAYS_INLINE USED void OP(set_encoder)(PTR map, ENC_PTR enc) {
  return map->encoder(enc);
}

CNAME ALWAYS_INLINE USED void OP(set_decoder)(PTR map, DEC_PTR dec) {
  return map->decoder(dec);
}

CNAME ALWAYS_INLINE USED ENC_PTR OP(get_encoder)(PTR map) {
  return map->encoder();
}

CNAME ALWAYS_INLINE USED DEC_PTR OP(get_decoder)(PTR map) {
  return map->decoder();
}

CNAME ALWAYS_INLINE USED void OP(free)(PTR map) {
  delete map;
}

CNAME ALWAYS_INLINE USED VAL_TYPE OP(read_encoded)(PTR map, SIZE_TYPE i) {
  return map->read_encoded(i);
}

CNAME ALWAYS_INLINE USED VAL_TYPE OP(read)(PTR map, KEY_TYPE key) {
  return map->read(key);
}

CNAME ALWAYS_INLINE USED PTR OP(write_encoded)(PTR map,
                                               SIZE_TYPE i,
                                               VAL_TYPE value) {
  map->write_encoded(i, value);
  return map;
}

CNAME ALWAYS_INLINE USED PTR OP(write)(PTR map, KEY_TYPE key, VAL_TYPE value) {
  map->write(key, value);
  return map;
}

CNAME ALWAYS_INLINE USED VAL_TYPE *OP(get_encoded)(PTR map, SIZE_TYPE i) {
  return map->get_encoded(i);
}

CNAME ALWAYS_INLINE USED VAL_TYPE *OP(get)(PTR map, KEY_TYPE key) {
  return map->get(key);
}

CNAME ALWAYS_INLINE USED PTR OP(copy)(PTR map) {
  return map->copy();
}

CNAME ALWAYS_INLINE USED PTR OP(remove_encoded)(PTR map, SIZE_TYPE i) {
  map->remove_encoded(i);
  return map;
}

CNAME ALWAYS_INLINE USED PTR OP(remove)(PTR map, KEY_TYPE key) {
  map->remove(key);
  return map;
}

CNAME ALWAYS_INLINE USED PTR OP(insert_encoded)(PTR map, SIZE_TYPE i) {
  map->insert_encoded(i);
  return map;
}

CNAME ALWAYS_INLINE USED PTR OP(insert)(PTR map, KEY_TYPE key) {
  map->insert(key);
  return map;
}

CNAME ALWAYS_INLINE USED PTR OP(insert_value_encoded)(PTR map,
                                                      SIZE_TYPE i,
                                                      VAL_TYPE value) {
  map->insert_encoded(i, value);
  return map;
}

CNAME ALWAYS_INLINE USED PTR OP(insert_value)(PTR map,
                                              KEY_TYPE key,
                                              VAL_TYPE value) {
  map->insert(key, value);
  return map;
}

CNAME ALWAYS_INLINE USED bool OP(has_encoded)(PTR map, SIZE_TYPE i) {
  return map->has_encoded(i);
}

CNAME ALWAYS_INLINE USED bool OP(has)(PTR map, KEY_TYPE key) {
  return map->has(key);
}

CNAME ALWAYS_INLINE USED size_t OP(size)(PTR map) {
  return map->size();
}

CNAME ALWAYS_INLINE USED PTR OP(clear)(PTR map) {
  map->clear();
  return map;
}

CNAME ALWAYS_INLINE USED void OP(begin)(ITER_PTR iter, PTR map) {
  map->begin(iter);
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
#undef ITER_TYPE
#undef ITER_PTR
