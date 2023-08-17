#ifndef COMMON_ASSERT_H
#define COMMON_ASSERT_H
#pragma once

#include <cassert>
#include <cstdio>
#include <string>
#include <type_traits>

#define _MEMOIR_ASSERT(c, msg, pretty_func, pretty_line)                       \
  if (!(c)) {                                                                  \
    fprintf(stderr, "\n");                                                     \
    fprintf(stderr, "\x1b[31m====================================\n");         \
    fprintf(stderr, "\x1b[1;31mMemOIR Assert Failed!\x1b[0m\n");               \
    fprintf(stderr, "  \x1b[1;33m%s\x1b[0m\n", msg);                           \
    fprintf(stderr, "  in \x1b[1;35m%s\x1b[0m\n", pretty_func);                \
    fprintf(stderr, "  at \x0b[1;35m%d\x1b[0m\n", pretty_line);                \
    fprintf(stderr, "\x1b[31m====================================\n\n");       \
    assert(c);                                                                 \
  }

#define MEMOIR_ASSERT(c, msg)                                                  \
  _MEMOIR_ASSERT(c, msg, __PRETTY_FUNCTION__, __LINE__)

#define MEMOIR_UNREACHABLE(msg) MEMOIR_ASSERT(false, msg)

#define MEMOIR_NULL_CHECK(v, msg) MEMOIR_ASSERT((v != nullptr), msg)

#define MEMOIR_SANITIZE(v, msg) sanitize(v, msg, __PRETTY_FUNCTION__, __LINE__)

template <typename T>
inline typename std::enable_if_t<
    std::is_pointer_v<T>,
    std::add_lvalue_reference_t<std::remove_pointer_t<T>>>
sanitize(T t,
         const char *message,
         const char *pretty_func = "",
         int pretty_line = 0) {
  _MEMOIR_ASSERT((t != nullptr), message, pretty_func, pretty_line);
  return *t;
}

#endif
