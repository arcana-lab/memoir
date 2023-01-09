#ifndef COMMON_ASSERT_H
#define COMMON_ASSERT_H
#pragma once

#include <cassert>
#include <cstdio>

#define MEMOIR_ASSERT(c, msg)                                                  \
  if (!c) {                                                                    \
    fprintf(stderr, "\n");                                                     \
    fprintf(stderr, "\x1b[31m====================================\n");         \
    fprintf(stderr, "\x1b[1;31mMemOIR Assert Failed!\x1b[0m\n");               \
    fprintf(stderr, "\x1b[0;33m%s\x1b[0m\n", msg);                             \
    fprintf(stderr, "  in \x1b[1;35m%s\x1b[0m\n", __PRETTY_FUNCTION__);        \
    fprintf(stderr, "  at \x0b[1;35m%s\x1b[0m\n", __LINE__);                   \
    assert(c);                                                                 \
  }

#define MEMOIR_UNREACHABLE(msg) MEMOIR_ASSERT(false, msg)

#define MEMOIR_NULL_CHECK(v, msg) MEMOIR_ASSERT((v != nullptr), msg)

#endif
