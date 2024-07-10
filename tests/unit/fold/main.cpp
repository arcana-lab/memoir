#include <cstdio>

#include "cmemoir/cmemoir.h"
#include "cmemoir/test.hpp"

using namespace memoir;

uint32_t sum_seq(uint32_t accum, size_t i, uint32_t v) {
  return accum + v;
}

uint32_t sum_seq_times(uint32_t accum, size_t i, uint32_t v, uint32_t x) {
  return accum + v * x;
}

uint32_t sum_seq_times_mut(uint32_t accum, size_t i, uint32_t v, uint32_t *x) {
  return accum + v * ((*x)++);
}

uint32_t sum_assoc(uint32_t accum, uint32_t k, uint32_t v) {
  return accum + k + v;
}

int main() {

  TEST(seq) {

    auto seq = memoir_allocate_sequence(memoir_u32_t, 100);

    for (size_t i = 0; i < 100; ++i) {
      memoir_index_write(u32, 1, seq, i);
    }

    auto sum = memoir_fold(u32, seq, 0, sum_seq);

    EXPECT(sum == 100, "Sum incorrect!");
  }

  TEST(assoc) {
    auto assoc = memoir_allocate_assoc_array(memoir_u32_t, memoir_u32_t);

    memoir_assoc_insert(assoc, 10);
    memoir_assoc_write(u32, 1, assoc, 10);
    memoir_assoc_insert(assoc, 20);
    memoir_assoc_write(u32, 2, assoc, 20);
    memoir_assoc_insert(assoc, 30);
    memoir_assoc_write(u32, 3, assoc, 30);

    auto sum = memoir_fold(u32, assoc, 0, sum_assoc);

    EXPECT(sum == (10 + 1 + 2 + 20 + 3 + 30), "Sum incorrect!");
  }

  TEST(close_immut_scalar) {
    auto seq = memoir_allocate_sequence(memoir_u32_t, 100);

    for (size_t i = 0; i < 100; ++i) {
      memoir_index_write(u32, 1, seq, i);
    }

    const uint32_t x = 10;

    auto sum = memoir_fold(u32, seq, 0, sum_seq_times, x);

    EXPECT(sum == 1000, "Sum incorrect!");
  }

  TEST(close_mut_scalar) {
    auto seq = memoir_allocate_sequence(memoir_u32_t, 10);

    for (size_t i = 0; i < 10; ++i) {
      memoir_index_write(u32, 1, seq, i);
    }

    uint32_t x;
    x = 1;

    auto sum = memoir_fold(u32, seq, 0, sum_seq_times_mut, &x);

    printf("%u", x);

    EXPECT(sum == (1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10), "Sum incorrect!");
    EXPECT(x == 11, "x incorrect!");
  }
}
