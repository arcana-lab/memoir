#include <backend/utilities.h>

#include <backend/abseil_flat_hash_map/definition.hpp>

#define KEY_CODE CODE_0
#define KEY_TYPE TYPE_0
#define VAL_CODE CODE_1
#define VAL_TYPE TYPE_1

#define IMPL abseil_flat_hash_map
#define PREFIX CAT(KEY_CODE, CAT(_, CAT(VAL_CODE, CAT(_, IMPL))))

#define TYPE CAT(PREFIX, _t)
#define PTR CAT(PREFIX, _p)

#define ITER_TYPE CAT(PREFIX, _iter_t)
#define ITER_PTR CAT(PREFIX, _iter_p)

#define OP(op) CAT(CAT(PREFIX, __), op)

CNAME ALWAYS_INLINE USED PTR OP(allocate)() {
  return new TYPE();
}

CNAME ALWAYS_INLINE USED void OP(free)(PTR map) {
  delete map;
}

CNAME ALWAYS_INLINE USED VAL_TYPE OP(read)(PTR map, KEY_TYPE key) {
  return map->read(key);
}

CNAME ALWAYS_INLINE USED PTR OP(write)(PTR map, KEY_TYPE key, VAL_TYPE value) {
  map->write(key, value);
  return map;
}

CNAME ALWAYS_INLINE USED VAL_TYPE *OP(get)(PTR map, KEY_TYPE key) {
  return map->get(key);
}

CNAME ALWAYS_INLINE USED PTR OP(copy)(PTR map) {
  return PTR(map->copy());
}

CNAME ALWAYS_INLINE USED PTR OP(remove)(PTR map, KEY_TYPE key) {
  map->remove(key);
  return map;
}

CNAME ALWAYS_INLINE USED PTR OP(insert)(PTR map, KEY_TYPE key) {
  map->insert(key);
  return map;
}

CNAME ALWAYS_INLINE USED PTR OP(insert_value)(PTR map,
                                              KEY_TYPE key,
                                              VAL_TYPE val) {
  map->insert_value(key, val);
  return map;
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

#undef IMPL
#undef KEY_CODE
#undef KEY_TYPE
#undef VAL_CODE
#undef VAL_TYPE
#undef PREFIX
#undef ITER_TYPE
#undef ITER_PTR
