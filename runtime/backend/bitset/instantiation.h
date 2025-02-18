// EXPECTS:
//  CODE_0, the element type code
//  TYPE_0, the element C type

#include <backend/utilities.h>

#include <backend/bitset/definition.hpp>

#define KEY_CODE CODE_0
#define KEY_TYPE TYPE_0

#define SIZE_TYPE size_t

#define IMPL bitset
#define PREFIX CAT(KEY_CODE, CAT(_, IMPL))

#define TYPE CAT(PREFIX, _t)
#define PTR CAT(PREFIX, _p)

#define ITER_TYPE CAT(PREFIX, _iter_t)
#define ITER_PTR CAT(PREFIX, _iter_p)

#define OP(op) CAT(CAT(PREFIX, __), op)

cname alwaysinline used PTR OP(allocate)() {
  return new TYPE();
}

cname alwaysinline used void OP(free)(PTR set) {
  delete set;
}

cname alwaysinline used PTR OP(copy)(PTR set) {
  return PTR(set->copy());
}

cname alwaysinline used PTR OP(remove)(PTR set, SIZE_TYPE key) {
  set->remove(key);
  return set;
}

cname alwaysinline used PTR OP(insert)(PTR set, SIZE_TYPE key) {
  set->insert(key);
  return set;
}

cname alwaysinline used PTR OP(insert_input)(PTR set, PTR set2) {
  set->insert_input(set2);
  return set;
}

cname alwaysinline used bool OP(has)(PTR set, SIZE_TYPE key) {
  return set->has(key);
}

cname alwaysinline used size_t OP(size)(PTR set) {
  return set->size();
}

cname alwaysinline used PTR OP(clear)(PTR set) {
  set->clear();
  return set;
}

cname alwaysinline used void OP(begin)(ITER_PTR iter, PTR set) {
  set->begin(iter);
}

cname alwaysinline used bool OP(next)(ITER_PTR iter) {
  return iter->next();
}

#undef KEY_CODE
#undef KEY_TYPE
#undef SIZE_TYPE
#undef IMPL
#undef PREFIX
#undef ITER_TYPE
#undef ITER_PTR
