#ifndef MEMOIR_TESTS_MEMOIRTESTING_H
#define MEMOIR_TESTS_MEMOIRTESTING_H

#include <csignal>
#include <cstdio>
#include <cstdlib>

namespace memoir::test {

// ===== API =====
#define TEST(NAME)                                                             \
  if (memoir::test::tests > 0) {                                               \
    if (memoir::test::test_passing) {                                          \
      fprintf(stderr, "\r                                              \r");   \
      ++memoir::test::passed;                                                  \
    } else {                                                                   \
      ++memoir::test::failed;                                                  \
    }                                                                          \
  }                                                                            \
  fprintf(stderr, "\e[1m%s \e[0m", #NAME);                                     \
  ++memoir::test::tests;                                                       \
  memoir::test::test_passing = true;

#define EXPECT(FLAG, ERROR)                                                    \
  if (memoir::test::test_passing and not memoir::test::expect(FLAG, ERROR)) {  \
    memoir::test::test_passing = false;                                        \
  }

// ===== Internals =====
static unsigned int tests = 0;
static unsigned int passed = 0;
static unsigned int failed = 0;
static bool test_passing = false;

__attribute__((destructor)) void end() {
  if (memoir::test::test_passing) {
    fprintf(stderr, "\r                                              \r");
    ++memoir::test::passed;
  } else {
    ++memoir::test::failed;
  }
  fprintf(stderr,
          "\e[32;1m%u PASSED\e[0m, \e[31;1m%u FAILED\e[0m of \e[1m%u TESTS\n",
          passed,
          failed,
          tests);
  return;
}

__attribute__((optnone)) bool expect(bool test, const char *error) {
  // If the test failed, print the error.
  if (not test) {
    fprintf(stderr, "\e[31;1mFAILED\e[0m\n  \e[33;1mREASON:\e[0m %s\n", error);
  }
  return test;
}

__attribute__((optnone)) void segfault_handler(int sig) {
  fprintf(stderr, "\e[31;1mFAILED, SEGFAULT!\n");
  memoir::test::test_passing = false;
  exit(2);
}

__attribute__((constructor)) void begin() {
  std::signal(SIGSEGV, segfault_handler);
}

} // namespace memoir::test

#endif // MEMOIR_TESTS_MEMOIRTESTING_H
