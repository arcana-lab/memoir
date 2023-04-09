#include <iostream>

#include "cmemoir.h"

using namespace memoir;

#define VAL0 1
#define VAL1 2
#define VAL2 3
#define VAL3 4
#define VAL4 5
#define VAL5 6

int main(int argc, char **argv) {

  int right = 4;

  printf("\nInitializing sequences\n");

  auto seq0 = memoir_allocate_sequence(memoir_u32_t, 3);

  memoir_index_write(u32, VAL0, seq0, 0);
  memoir_index_write(u32, VAL1, seq0, 1);
  memoir_index_write(u32, VAL2, seq0, 2);

  auto seq1 = memoir_allocate_sequence(memoir_u32_t, 3);

  memoir_index_write(u32, VAL3, seq1, 0);
  memoir_index_write(u32, VAL4, seq1, 1);
  memoir_index_write(u32, VAL5, seq1, 2);

  printf("\nReading sequences\n");

  auto read00 = memoir_index_read(u32, seq0, 0);
  auto read01 = memoir_index_read(u32, seq0, 1);
  auto read02 = memoir_index_read(u32, seq0, 2);
  auto read10 = memoir_index_read(u32, seq1, 0);
  auto read11 = memoir_index_read(u32, seq1, 1);
  auto read12 = memoir_index_read(u32, seq1, 2);

  printf("Result:\n");
  printf(" Sequence 1:\n");
  printf("  HEAD -> %d\n", read00);
  printf("       -> %d\n", read01);
  printf("       -> %d\n", read02);
  printf(" Sequence 2:\n");
  printf(" HEAD  -> %d\n", read10);
  printf("       -> %d\n", read11);
  printf("       -> %d\n", read12);

  printf("Expected:\n");
  printf(" Sequence 1:\n");
  printf("  HEAD -> %d\n", VAL0);
  printf("       -> %d\n", VAL1);
  printf("       -> %d\n", VAL2);
  printf(" Sequence 2:\n");
  printf("  HEAD -> %d\n", VAL3);
  printf("       -> %d\n", VAL4);
  printf("       -> %d\n", VAL5);

  printf("\n Joining sequences\n");

  auto join = memoir_join(seq0, seq1);

  printf("\nSlicing sequences\n");

  auto slice = memoir_sequence_slice(join, 0, read11);

  printf("\nReading sequences\n");

  for (auto i = 0; i < right; i++) {
    printf("%d\n", memoir_index_read(u32, slice, i));
  }

  return 0;
}
