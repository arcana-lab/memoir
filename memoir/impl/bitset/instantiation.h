// EXPECTS:
//  CODE_0, the element type code
//  TYPE_0, the element C type

#include <memoir/impl/utilities.h>

#include <memoir/impl/bitset/definition.hpp>

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

CNAME ALWAYS_INLINE USED PTR OP(allocate)() {
  return new TYPE();
}

CNAME ALWAYS_INLINE USED void OP(free)(PTR set) {
  delete set;
}

CNAME ALWAYS_INLINE USED PTR OP(copy)(PTR set) {
  return PTR(set->copy());
}

CNAME ALWAYS_INLINE USED PTR OP(remove)(PTR set, SIZE_TYPE key) {
  set->remove(key);
  return set;
}

CNAME ALWAYS_INLINE USED PTR OP(insert)(PTR set, SIZE_TYPE key) {
  set->insert(key);
  return set;
}

CNAME ALWAYS_INLINE USED PTR OP(insert_input)(PTR set, PTR set2) {
  set->insert_input(set2);
  return set;
}

CNAME ALWAYS_INLINE USED bool OP(has)(PTR set, SIZE_TYPE key) {
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
#undef SIZE_TYPE
#undef IMPL
#undef PREFIX
#undef ITER_TYPE
#undef ITER_PTR
