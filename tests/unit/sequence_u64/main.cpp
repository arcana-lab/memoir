#include <iostream>

#include "cmemoir/cmemoir.h"

using namespace memoir;

#define VAL0 10
#define VAL1 20
#define VAL2 30

int main() {
  printf("\nInitializing sequence\n");

  auto seq = memoir_allocate_sequence(memoir_u64_t, 3);

  memoir_index_write(u64, VAL0, seq, 0);
  memoir_index_write(u64, VAL1, seq, 1);
  memoir_index_write(u64, VAL2, seq, 2);

  printf("\nReading sequence\n");

  auto read0 = memoir_index_read(u64, seq, 0);
  auto read1 = memoir_index_read(u64, seq, 1);
  auto read2 = memoir_index_read(u64, seq, 2);

  printf(" Result:\n");
  printf("  HEAD -> %d\n", read0);
  printf("       -> %d\n", read1);
  printf("       -> %d\n\n", read2);

  printf("Expected:\n");
  printf("  HEAD -> %d\n", VAL0);
  printf("       -> %d\n", VAL1);
  printf("       -> %d\n\n", VAL2);
}
