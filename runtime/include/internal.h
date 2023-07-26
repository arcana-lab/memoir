#ifndef MEMOIR_INTERNAL_H
#define MEMOIR_INTERNAL_H

#include <cstdio>
#include <memory>

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
  MEMOIR_ASSERT((obj->get_type() != nullptr),                                  \
                "Type is NULL, type check failed");                            \
  MEMOIR_ASSERT((obj->get_type()->getCode() == type_code),                     \
                "Type code mismatch, expected: " #type_code)

#define MEMOIR_OBJECT_CHECK(obj)                                               \
  MEMOIR_ASSERT((obj != nullptr), "Attempt to get NULL object");               \
  MEMOIR_ASSERT(is_object_type(obj->get_type()),                               \
                "Element is not a nested object")

#define MEMOIR_STRUCT_CHECK(obj)                                               \
  MEMOIR_ASSERT((obj != nullptr), "Attempt to get NULL object");               \
  MEMOIR_ASSERT(is_struct_type(obj->get_type()), "Element is not a struct type")

#define MEMOIR_COLLECTION_CHECK(obj)                                           \
  MEMOIR_ASSERT((obj != nullptr), "Attempt to get NULL object");               \
  MEMOIR_ASSERT(is_collection_type(obj->get_type()),                           \
                "Element is not of collection type")

#define MEMOIR_INTEGER_CHECK(obj, bw, s, name)                                 \
  MEMOIR_TYPE_CHECK(obj, TypeCode::IntegerTy);                                 \
  MEMOIR_ASSERT(((static_cast<IntegerType *>(type)->bitwidth == bw)            \
                 && (static_cast<IntegerType *>(type)->is_signed == s)),       \
                "Element is not of type " #name)

#define MEMOIR_WARNING(msg)                                                    \
  fprintf(stderr, "\x1b[31mWARNING:\x1b[0m %s\n", msg);

#endif
