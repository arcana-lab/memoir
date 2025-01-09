#ifndef COMMON_ASSERT_H
#define COMMON_ASSERT_H

#include <cassert>
#include <cstdio>
#include <string>
#include <type_traits>

#include "memoir/support/Print.hpp"

#define _MEMOIR_ASSERT(pretty_func, pretty_line, c, msg...)                    \
  if (!(c)) {                                                                  \
    llvm::memoir::println("\n");                                               \
    llvm::memoir::println("\x1b[31m====================================");     \
    llvm::memoir::println("\x1b[1;31mMemOIR Assert Failed!\x1b[0m");           \
    llvm::memoir::println("  \x1b[1;33m", msg, "\x1b[0m");                     \
    llvm::memoir::println("  in \x1b[1;35m", pretty_func, "\x1b[0m");          \
    llvm::memoir::println("  at \x1b[1;35m", pretty_line, "\x1b[0m");          \
    llvm::memoir::println(                                                     \
        "\x1b[31m====================================\x1b[0m");                \
    llvm::memoir::println();                                                   \
    assert(c);                                                                 \
  }

#define MEMOIR_ASSERT(c, msg...)                                               \
  _MEMOIR_ASSERT(__PRETTY_FUNCTION__, __LINE__, c, msg)

#define MEMOIR_UNREACHABLE(msg) MEMOIR_ASSERT(false, msg)

#define MEMOIR_NULL_CHECK(v, msg) MEMOIR_ASSERT((v != nullptr), msg)

#define MEMOIR_SANITIZE(v, msg)                                                \
  llvm::memoir::sanitize(v, msg, __PRETTY_FUNCTION__, __LINE__)

namespace llvm::memoir {

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

} // namespace llvm::memoir

#endif
