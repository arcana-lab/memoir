#include <iostream>

#include "cmemoir/cmemoir.h"

using namespace memoir;

#define N 1000000

int main() {
  auto seq = memoir_allocate_sequence(memoir_u64_t, 0);

  for (int i = 0; i < N; ++i) {
    memoir_index_write(u64, 10 * i, seq, i);
  }

  auto cpy = memoir_seq_copy(seq, 0, 100);

  printf("Result:\n");
  for (int i = 0; i < 10; ++i) {
    printf("%d,", memoir_index_read(u64, cpy, i));
  }
  printf("\n");

  printf("Expected:\n");
  for (int i = 0; i < 10; ++i) {
    printf("%d,", 10 * i);
  }
  printf("\n");
}
