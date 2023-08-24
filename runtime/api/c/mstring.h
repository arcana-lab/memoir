#ifndef MEMOIR_API_MSTRING_H
#define MEMOIR_API_MSTRING_H
#pragma once

#include "cmemoir.h"

using namespace memoir;

#define memoir_string_t memoir_sequence_type(memoir_u8_t)

// Don't look at the details here, only the next 4 lines.
// memoir_strcmp can be used as:
//   memoir_strcmp(str1, begin1, str2, begin2)
//   -- or --
//   memoir_strcmp(str, begin1, begin2)
#define memoir_strcmp(a, b, ...) memoir_strcmp_(a, b, ##__VA_ARGS__, 4, 3, 2)
#define memoir_strcmp_(a, b, c, d, n, ...) memoir_strcmp##n(a, b, c, d)
#define memoir_strcmp4(str, a, str2, b) memoir_strcmp__diff(str, a, str2, b)
#define memoir_strcmp3(str, a, b, ...) memoir_strcmp__same(str, a, b)
#define memoir_strcmp2(str, str2, ...) memoir_strcmp__start(str, str2)

#define memoir_strtok(str, delims, /*size_t*/ last, /*size_t*/ next)           \
  do {                                                                         \
    /* Scan leading delimiters */                                              \
    last += memoir_strspn(str, delims, last);                                  \
    if (memoir_index_read(u8, str, last) == '\0') {                            \
      last = 0;                                                                \
      break;                                                                   \
    }                                                                          \
                                                                               \
    /* Find the end of the token */                                            \
    next = last;                                                               \
    last = memoir_strpbrk(str, delims, next);                                  \
    if (last != 0) {                                                           \
      /* Terminate the token */                                                \
      memoir_index_write(u8, '\0', str, last++);                               \
    }                                                                          \
  } while (0)

extern int memoir_strcmp__start(Collection *str, Collection *str2);

extern int memoir_strcmp__same(Collection *str, size_t a, size_t b);

extern int memoir_strcmp__diff(Collection *str,
                               size_t a,
                               Collection *str2,
                               size_t b);

extern size_t memoir_strlen(Collection *str, size_t start = 0);

/* extern size_t memoir_strtok(Collection *str, */
/*                             const char *delimiters, */
/*                             size_t last = 0); */

extern size_t memoir_strspn(Collection *str,
                            const char *accept,
                            size_t start = 0);

extern size_t memoir_strpbrk(Collection *str,
                             const char *accept,
                             size_t start = 0);

#endif
