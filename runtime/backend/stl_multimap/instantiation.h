// EXPECTS:
//  CODE_0, the element type code
//  TYPE_0, the element C type

#include <backend/utilities.h>

#include <backend/stl_multimap/definition.hpp>

#define KEY_CODE CODE_0
#define KEY_TYPE TYPE_0
#define VAL_CODE CODE_1
#define VAL_TYPE TYPE_1

#define PREFIX CAT(KEY_CODE, CAT(_, CAT(VAL_CODE, _stl_multimap)))

#define TYPE CAT(PREFIX, _t)
#define PTR CAT(PREFIX, _p)

#define ITER_TYPE CAT(PREFIX, _iter_t)
#define ITER_PTR CAT(PREFIX, _iter_p)

#define OP(op) CAT(CAT(PREFIX, __), op)

cname alwaysinline used PTR OP(allocate)() {
  return new TYPE();
}

cname alwaysinline used void OP(free)(PTR map) {
  delete map;
}

cname alwaysinline used VAL_TYPE OP(read)(PTR map, KEY_TYPE key, size_t index) {
  return map->read(key, index);
}

cname alwaysinline used PTR OP(write)(PTR map,
                                      KEY_TYPE key,
                                      size_t index,
                                      VAL_TYPE value) {
  map->write(key, index, value);
  return map;
}

cname alwaysinline used VAL_TYPE *OP(get)(PTR map, KEY_TYPE key, size_t index) {
  return map->get(key, index);
}

cname alwaysinline used PTR OP(copy)(PTR map) {
  return map->copy();
}

cname alwaysinline used PTR OP(remove)(PTR map, KEY_TYPE key, size_t index) {
  map->remove(key, index);
  return map;
}

cname alwaysinline used PTR OP(remove__1)(PTR map, KEY_TYPE key) {
  map->remove(key);
  return map;
}

cname alwaysinline used PTR OP(insert)(PTR map, KEY_TYPE key, size_t index) {
  map->insert(key, index);
  return map;
}

cname alwaysinline used PTR OP(insert_value)(PTR map,
                                             KEY_TYPE key,
                                             size_t index,
                                             VAL_TYPE value) {
  map->insert(key, index, value);
  return map;
}

cname alwaysinline used PTR OP(insert__1)(PTR map, KEY_TYPE key) {
  map->insert(key);
  return map;
}

cname alwaysinline used bool OP(has)(PTR map, KEY_TYPE key) {
  return map->has(key);
}

cname alwaysinline used size_t OP(size)(PTR map, KEY_TYPE key) {
  return map->size(key);
}

cname alwaysinline used size_t OP(size__1)(PTR map) {
  return map->size();
}

cname alwaysinline used PTR OP(clear)(PTR map, KEY_TYPE key) {
  map->clear(key);
  return map;
}

cname alwaysinline used PTR OP(clear__1)(PTR map) {
  map->clear();
  return map;
}

cname alwaysinline used void OP(begin)(ITER_PTR iter, PTR map, KEY_TYPE key) {
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
#undef ITER_TYPE
#undef ITER_PTR
