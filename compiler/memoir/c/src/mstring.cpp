#include "mstring.h"

int memoir_strcmp__start(Collection *str, Collection *str2) {
  memoir_assert_collection_type(memoir_string_t, str);
  memoir_assert_collection_type(memoir_string_t, str2);

  auto a = 0;
  auto b = 0;

  auto ac = memoir_index_read(u8, str, a);
  auto bc = memoir_index_read(u8, str2, b);
  while (ac != '\0' && (ac == bc)) {
    ac = memoir_index_read(u8, str, ++a);
    bc = memoir_index_read(u8, str2, ++b);
  }

  return (int)(a - b);
}

int memoir_strcmp__same(Collection *str, size_t a, size_t b) {
  memoir_assert_collection_type(memoir_string_t, str);

  auto aa = a;
  auto bb = b;

  auto ac = memoir_index_read(u8, str, a);
  auto bc = memoir_index_read(u8, str, b);
  while (ac != '\0' && (ac == bc)) {
    ac = memoir_index_read(u8, str, ++a);
    bc = memoir_index_read(u8, str, ++b);
  }

  return (int)((a - aa) - (b - bb));
}

int memoir_strcmp__diff(Collection *str, size_t a, Collection *str2, size_t b) {
  memoir_assert_collection_type(memoir_string_t, str);
  memoir_assert_collection_type(memoir_string_t, str2);

  auto aa = a;
  auto bb = b;

  auto ac = memoir_index_read(u8, str, a);
  auto bc = memoir_index_read(u8, str2, b);
  while (ac != '\0' && (ac == bc)) {
    ac = memoir_index_read(u8, str, ++a);
    bc = memoir_index_read(u8, str2, ++b);
  }
  return (int)((a - aa) - (b - bb));
}

size_t memoir_strlen(Collection *str, size_t start) {
  memoir_assert_collection_type(memoir_string_t, str);

  auto cur = start;

  auto v = memoir_index_read(u8, str, cur);
  while (v != '\0') {
    v = memoir_index_read(u8, str, ++cur);
  }

  return cur - start;
}

size_t memoir_strspn(Collection *str, const char *accept, size_t start) {
  size_t count = 0;

  size_t p = start;
  auto pc = memoir_index_read(u8, str, p);
  while (pc != '\0') {
    for (const char *a = accept;; ++a) {
      if (pc == *a) {
        break;
      }
      if (*a == '\0') {
        return count;
      }
    }
    ++count;
    pc = memoir_index_read(u8, str, ++p);
  }

  return count;
}

size_t memoir_strpbrk(Collection *str, const char *accept, size_t start) {
  size_t p = start;

  auto pc = memoir_index_read(u8, str, p);
  while (pc != '\0') {
    const char *a = accept;
    while (*a != '\0') {
      if (*a++ == pc) {
        return p;
      }
    }
    pc = memoir_index_read(u8, str, ++p);
  }

  return 0;
}

// size_t memoir_strtok(Collection *str, const char *delimiters, size_t last) {
//   /* Scan leading delimiters */
//   last += memoir_strspn(str, delimiters, last);
//   if (memoir_index_read(u8, str, last) == '\0') {
//     return 0;
//   }

//   /* Find the end of the token */
//   size_t token = last;
//   last = memoir_strpbrk(str, delimiters, token);
//   if (last != 0) {
//     /* Terminate the token */
//     memoir_index_write(u8, '\0', str, last);
//   }

//   return token;
// }
