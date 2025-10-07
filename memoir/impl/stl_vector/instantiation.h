// EXPECTS:
//  CODE_0, the element type code
//  TYPE_0, the element C type

#include "memoir/impl/utilities.h"

#include "memoir/impl/stl_vector/definition.hpp"

#define RESERVE_SIZE 4000 + 60 + 1

#define IMPL stl_vector
#define PREFIX CAT(CODE_0, CAT(_, IMPL))

#define TYPE CAT(PREFIX, _t)
#define PTR CAT(PREFIX, _p)

#define ITER_TYPE CAT(PREFIX, _iter_t)
#define ITER_PTR CAT(PREFIX, _iter_p)

#define RITER_TYPE CAT(PREFIX, _riter_t)
#define RITER_PTR CAT(PREFIX, _riter_p)

#define OP(op) CAT(CAT(PREFIX, __), op)

CNAME ALWAYS_INLINE USED PTR OP(allocate)(size_t num) {
  return new TYPE(num);
}

CNAME ALWAYS_INLINE USED void OP(free)(PTR vec) {
  delete vec;
}

CNAME ALWAYS_INLINE USED TYPE_0 OP(read)(PTR vec, size_t index) {
  return vec->read(index);
}

CNAME ALWAYS_INLINE USED PTR OP(write)(PTR vec, size_t index, TYPE_0 value) {
  vec->write(index, value);
  return vec;
}

// TODO: wrap this in a preprocessor if check
#if NOT_EQUAL(CODE_0, boolean)
CNAME ALWAYS_INLINE USED TYPE_0 *OP(get)(PTR vec, size_t index) {
  return vec->get(index);
}

CNAME ALWAYS_INLINE USED PTR OP(copy)(PTR vec) {
  return PTR(vec->copy());
}

CNAME ALWAYS_INLINE USED PTR OP(copy_range)(PTR vec,
                                            size_t begin_index,
                                            size_t end_index) {
  return PTR(vec->copy(begin_index, end_index));
}

CNAME ALWAYS_INLINE USED PTR OP(remove)(PTR vec, size_t index) {
  vec->remove(index);
  return vec;
}

CNAME ALWAYS_INLINE USED PTR OP(remove_range)(PTR vec,
                                              size_t begin_index,
                                              size_t end_index) {
  vec->remove(begin_index, end_index);
  return vec;
}
CNAME ALWAYS_INLINE USED PTR OP(insert)(PTR vec, size_t start) {
  vec->insert(start);
  return vec;
}

CNAME ALWAYS_INLINE USED PTR OP(insert_value)(PTR vec,
                                              size_t start,
                                              TYPE_0 value) {
  vec->insert(start, value);
  return vec;
}

CNAME ALWAYS_INLINE USED PTR OP(insert_input)(PTR vec, size_t start, PTR vec2) {
  vec->insert(start, vec2);
  return vec;
}

CNAME ALWAYS_INLINE USED PTR OP(insert_input_range)(PTR vec,
                                                    size_t start,
                                                    PTR vec2,
                                                    size_t begin,
                                                    size_t end) {
  vec->insert(start, vec2, begin, end);
  return vec;
}

#endif

CNAME ALWAYS_INLINE USED size_t OP(size)(PTR vec) {
  return vec->size();
}

CNAME ALWAYS_INLINE USED bool OP(has)(PTR vec, size_t index) {
  return index < vec->size();
}

CNAME ALWAYS_INLINE USED PTR OP(clear)(PTR vec) {
  vec->clear();
  return vec;
}

CNAME ALWAYS_INLINE USED void OP(begin)(ITER_PTR iter, PTR vec) {
  vec->begin(iter);
}
CNAME ALWAYS_INLINE USED bool OP(next)(ITER_PTR iter) {
  return iter->next();
}
CNAME ALWAYS_INLINE USED void OP(rbegin)(RITER_PTR iter, PTR vec) {
  vec->rbegin(iter);
}
CNAME ALWAYS_INLINE USED bool OP(rnext)(RITER_PTR iter) {
  return iter->next();
}

#undef RESERVE_SIZE
#undef IMPL
#undef PREFIX
#undef TYPE
#undef PTR
#undef ITER_TYPE
#undef ITER_PTR
#undef RITER_TYPE
#undef RITER_PTR
