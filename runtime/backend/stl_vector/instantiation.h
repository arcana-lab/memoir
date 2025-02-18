// EXPECTS:
//  CODE_0, the element type code
//  TYPE_0, the element C type

#include "backend/utilities.h"

#include "backend/stl_vector/definition.hpp"

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

cname alwaysinline used PTR OP(allocate)(size_t num) {
  return new TYPE(num);
}

cname alwaysinline used void OP(free)(PTR vec) {
  delete vec;
}

cname alwaysinline used TYPE_0 OP(read)(PTR vec, size_t index) {
  return vec->read(index);
}

cname alwaysinline used PTR OP(write)(PTR vec, size_t index, TYPE_0 value) {
  vec->write(index, value);
  return vec;
}

// TODO: wrap this in a preprocessor if check
#if NOT_EQUAL(CODE_0, boolean)
cname alwaysinline used TYPE_0 *OP(get)(PTR vec, size_t index) {
  return vec->get(index);
}

cname alwaysinline used PTR OP(copy)(PTR vec) {
  return PTR(vec->copy());
}

cname alwaysinline used PTR OP(copy_range)(PTR vec,
                                           size_t begin_index,
                                           size_t end_index) {
  return PTR(vec->copy(begin_index, end_index));
}

cname alwaysinline used PTR OP(remove)(PTR vec, size_t index) {
  vec->remove(index);
  return vec;
}

cname alwaysinline used PTR OP(remove_range)(PTR vec,
                                             size_t begin_index,
                                             size_t end_index) {
  vec->remove(begin_index, end_index);
  return vec;
}
cname alwaysinline used PTR OP(insert)(PTR vec, size_t start) {
  vec->insert(start);
  return vec;
}

cname alwaysinline used PTR OP(insert_value)(PTR vec,
                                             size_t start,
                                             TYPE_0 value) {
  vec->insert(start, value);
  return vec;
}

cname alwaysinline used PTR OP(insert_input)(PTR vec, size_t start, PTR vec2) {
  vec->insert(start, vec2);
  return vec;
}

cname alwaysinline used PTR OP(insert_input_range)(PTR vec,
                                                   size_t start,
                                                   PTR vec2,
                                                   size_t begin,
                                                   size_t end) {
  vec->insert(start, vec2, begin, end);
  return vec;
}

#endif

cname alwaysinline used size_t OP(size)(PTR vec) {
  return vec->size();
}

cname alwaysinline used PTR OP(clear)(PTR vec) {
  vec->clear();
  return vec;
}

cname alwaysinline used void OP(begin)(ITER_PTR iter, PTR vec) {
  vec->begin(iter);
}
cname alwaysinline used bool OP(next)(ITER_PTR iter) {
  return iter->next();
}
cname alwaysinline used void OP(rbegin)(RITER_PTR iter, PTR vec) {
  vec->rbegin(iter);
}
cname alwaysinline used bool OP(rnext)(RITER_PTR iter) {
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
