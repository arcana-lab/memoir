#ifndef MEMOIR_INTERNAL_H
#define MEMOIR_INTERNAL_H

#include <cstdio>

#define MEMOIR_ASSERT(cond, msg)                                               \
  if (!cond) {                                                                 \
    fprintf(stderr, "\x1b[31m-----[ MemOIR Assert Failed ]-----\x1b[0m\n");    \
    fprintf(stderr, "%s @ line %d\n", __FILE__, __LINE__);                     \
    fprintf(stderr, "in %s\n", __PRETTY_FUNCTION__);                           \
    fprintf(stderr, "%s (%s is false)\n", msg, #cond);                         \
    fprintf(stderr, "\x1b[31m----------------------------------\x1b[0m\n");    \
    exit(EXIT_FAILURE);                                                        \
  }

#define MEMOIR_UNREACHABLE(msg) MEMOIR_ASSERT(false, msg);

#define MEMOIR_ACCESS_CHECK(obj)                                               \
  MEMOIR_ASSERT((obj != nullptr), "Attempt to access NULL object")

#define MEMOIR_TYPE_CHECK(obj, type_code)                                      \
  MEMOIR_ASSERT((obj != nullptr), "Attempt to check type of NULL object");     \
  auto type = obj->get_type();                                                 \
  MEMOIR_ASSERT((type != nullptr), "Type is NULL, type check failed");         \
  MEMOIR_ASSERT((type->getCode() == type_code),                                \
                "Type code mismatch, expected: " #type_code)

#define MEMOIR_OBJECT_CHECK(obj)                                               \
  MEMOIR_ASSERT((obj != nullptr), "Attempt to get NULL object");               \
  MEMOIR_ASSERT(is_object_type(obj->get_type()),                               \
                "Element is not a nested object")

#define MEMOIR_INTEGER_CHECK(obj, bw, s, name)                                 \
  MEMOIR_TYPE_CHECK(obj, TypeCode::IntegerTy);                                 \
  auto integer_type = static_cast<IntegerType *>(type);                        \
  MEMOIR_ASSERT(                                                               \
      ((integer_type->bitwidth == bw) && (integer_type->is_signed == s)),      \
      "Element is not of type " #name)

#endif
