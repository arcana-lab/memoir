#include "cmemoir.h"

#define memoir_string_t memoir_sequence_type(memoir_u8_t)

// Don't look at the details here, only the next 4 lines.
// memoir_strcmp can be used as:
//   memoir_strcmp(str1, begin1, str2, begin2)
//   -- or --
//   memoir_strcmp(str, begin1, begin2)
#define memoir_strcmp(str, a, b, ...)                                          \
  memoir_strcmp_(str, a, ##__VA_ARGS__, 4, 3)
#define memoir_strcmp_(a, b, c, d, n, ...) memoir_strcmp##n(a, b, c, d)
#define memoir_strcmp4(str, a, str2, b) memoir_strcmp__diff(str, a, str2, b)
#define memoir_strcmp3(str, a, b, ...) memoir_strcmp__same(str, a, b)

uint32_t memoir_strcmp__same(Collection *str, size_t a, size_t b) {
  memoir_assert_collection_type(memoir_string_t, str);

  auto ac = memoir_index_read(u8, str, a);
  auto bc = memoir_index_read(u8, str, b);
  while (ac != '\0' && (ac == bc)) {
    ac = memoir_index_read(u8, str, a++);
    bc = memoir_index_read(u8, str, b++);
  }

  return (int)(a - b);
}

bool memoir_strcmp__diff(Collection *str,
                         size_t a,
                         Collection *str2,
                         size_t b) {
  memoir_assert_collection_type(memoir_string_t, str);
  memoir_assert_collection_type(memoir_string_t, str2);

  auto ac = memoir_index_read(u8, str, a);
  auto bc = memoir_index_read(u8, str2, b);
  while (ac != '\0' && (ac == bc)) {
    ac = memoir_index_read(u8, str, a++);
    bc = memoir_index_read(u8, str2, b++);
  }
  return (int)(a - b);
}

size_t memoir_strtok(Collection *str, const char *delimiters, size_t last = 0) {

  /* Instead of saving this value as a global variable, we will handle the wrap
   * case  explicitly*/
  if (last != 0) {
    last++;
  }

  /* Scan leading delimiters */
  last += strspn(str, delimiters);
  if (memoir_index_read(u8, str, last) == '\0') {
    return 0;
  }

  /* Find the end of the token */
  size_t token = last;
  last = memoir_strpbrk(str, delimiters, token);
  if (last != 0) {
    /* Terminate the token */
    memoir_index_write(u8, '\0', str, last);
  }
  return token;
}

size_t memoir_strspn(Collection *str, const char *accept, size_t start = 0) {
  size_t count = 0;

  size_t p = start;
  auto pc = memoir_index_read(u8, str, p);
  for (; pc != '\0';) {
    for (const char *a = accept; *a != '\0'; ++a) {
      if (pc == *a) {
        break;
      }
      if (*a == '\0') {
        return count;
      } else {
        ++count;
      }
    }
    pc = memoir_index_read(u8, str, ++p);
  }

  return count;
}

size_t memoir_strpbrk(Collection *str, const char *accept, size_t start = 0) {
  size_t p = start;

  auto pc = memoir_index_read(u8, str, p);
  while (pc != '\0') {
    const char *a = accept;
    while (*a != '\0') {
      if (*a++ == pc) {
        return p;
      }
      ++p;
    }
    pc = memoir_index_read(u8, str, p);
  }

  return 0;
}
