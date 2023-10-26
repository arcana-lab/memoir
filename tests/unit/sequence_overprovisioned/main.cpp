#include <cstdio>

#include "cmemoir/cmemoir.h"

using namespace memoir;

#define N 10
#define M 5

int main() {
  printf("\nInitializing sequence\n");

  auto seq = memoir_allocate_sequence(memoir_u64_t, N);

  for (auto i = 0; i < M; i++) {
    memoir_index_write(u64, i, seq, i);
  }

  printf("\nReading sequence\n");

  for (auto i = 0; i < M; i++) {
    auto read = memoir_index_read(u64, seq, i);
    printf("%u -> %lu\n", i, read);
  }
}
