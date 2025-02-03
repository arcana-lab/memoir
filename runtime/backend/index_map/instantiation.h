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
typedef IndexMap<KEY_TYPE, VAL_TYPE> TYPE;
typedef TYPE *PTR;

#define FWD_TYPE CAT(PREFIX, _fwd_t)
#define FWD_PTR CAT(PREFIX, _fwd_p)
typedef TYPE::ForwardMap FWD_TYPE;
typedef FWD_TYPE *FWD_PTR;

/* typedef KeyToIndexMap<KEY_TYPE> REV_TYPE; */
/* typedef REV_TYPE *REV_PTR; */

#define ITER_TYPE CAT(PREFIX, _iter_t)
#define ITER_PTR CAT(PREFIX, _iter_p)
typedef TYPE::iterator ITER_TYPE;
typedef ITER_TYPE *ITER_PTR;

typedef size_t SIZE_TYPE;

#define OP(op) CAT(CAT(PREFIX, __), op)

cname alwaysinline used PTR OP(allocate)() {
  return new TYPE();
}

cname alwaysinline used PTR OP(allocate_index)(FWD_PTR fwd) {
  return new TYPE(fwd);
}

cname alwaysinline used FWD_PTR OP(forward)(PTR map) {
  return map->forward();
}

#if 0
cname alwaysinline used PTR OP(reverse)(PTR map) {
  return map->reverse();
}
#endif

cname alwaysinline used void OP(free)(PTR map) {
  delete map;
}

cname alwaysinline used VAL_TYPE OP(read_index)(PTR map, SIZE_TYPE i) {
  return map->read_index(i);
}

cname alwaysinline used VAL_TYPE OP(read)(PTR map, KEY_TYPE key) {
  return map->read(key);
}

cname alwaysinline used PTR OP(write_index)(PTR map,
                                            SIZE_TYPE i,
                                            VAL_TYPE value) {
  map->write_index(i, value);
  return map;
}

cname alwaysinline used PTR OP(write)(PTR map, KEY_TYPE key, VAL_TYPE value) {
  map->write(key, value);
  return map;
}

cname alwaysinline used VAL_TYPE *OP(get_index)(PTR map, SIZE_TYPE i) {
  return map->get_index(i);
}

cname alwaysinline used VAL_TYPE *OP(get)(PTR map, KEY_TYPE key) {
  return map->get(key);
}

cname alwaysinline used PTR OP(copy)(PTR map) {
  return map->copy();
}

cname alwaysinline used PTR OP(remove_index)(PTR map, SIZE_TYPE i) {
  map->remove_index(i);
  return map;
}

cname alwaysinline used PTR OP(remove)(PTR map, KEY_TYPE key) {
  map->remove(key);
  return map;
}

cname alwaysinline used PTR OP(insert_index)(PTR map, SIZE_TYPE i) {
  map->insert_index(i);
  return map;
}

cname alwaysinline used PTR OP(insert)(PTR map, KEY_TYPE key) {
  map->insert(key);
  return map;
}

cname alwaysinline used PTR OP(insert_value_index)(PTR map,
                                                   SIZE_TYPE i,
                                                   VAL_TYPE value) {
  map->insert_index(i, value);
  return map;
}

cname alwaysinline used PTR OP(insert_value)(PTR map,
                                             KEY_TYPE key,
                                             VAL_TYPE value) {
  map->insert(key, value);
  return map;
}

cname alwaysinline used bool OP(has_index)(PTR map, SIZE_TYPE i) {
  return map->has_index(i);
}

cname alwaysinline used bool OP(has)(PTR map, KEY_TYPE key) {
  return map->has(key);
}

cname alwaysinline used size_t OP(size)(PTR map) {
  return map->size();
}

cname alwaysinline used PTR OP(clear)(PTR map) {
  map->clear();
  return map;
}

cname alwaysinline used void OP(begin)(ITER_PTR iter, PTR map) {
  map->begin(iter);
}

cname alwaysinline used bool OP(next)(ITER_PTR iter) {
  return iter->next();
}

#undef KEY_CODE
#undef KEY_TYPE
#undef VAL_CODE
#undef VAL_TYPE
#undef PREFIX
#undef TYPE
#undef PTR
#undef FWD_TYPE
#undef FWD_PTR
#undef ITER_TYPE
#undef ITER_PTR
