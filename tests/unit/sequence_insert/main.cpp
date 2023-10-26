#include <iostream>

#include "cmemoir/cmemoir.h"

using namespace memoir;

#define VAL0 (uint64_t)10
#define VAL1 (uint64_t)20
#define VAL2 (uint64_t)30
#define VAL3 (uint64_t)40
#define VAL4 (uint64_t)50
#define VAL5 (uint64_t)60

int main() {
  auto *seq = memoir_allocate_sequence(memoir_u64_t, 4);

  memoir_index_write(u64, VAL0, seq, 0);
  memoir_index_write(u64, VAL1, seq, 1);
  memoir_index_write(u64, VAL2, seq, 2);
  memoir_index_write(u64, VAL3, seq, 3);

  auto *seq2 = memoir_allocate_sequence(memoir_u64_t, 2);

  memoir_index_write(u64, VAL4, seq2, 0);
  memoir_index_write(u64, VAL5, seq2, 1);

  memoir_seq_insert_range(seq2, seq, 2);

  auto read0 = memoir_index_read(u64, seq, 0);
  auto read1 = memoir_index_read(u64, seq, 1);
  auto read2 = memoir_index_read(u64, seq, 2);
  auto read3 = memoir_index_read(u64, seq, 3);
  auto read4 = memoir_index_read(u64, seq, 4);
  auto read5 = memoir_index_read(u64, seq, 5);

  printf(" Result:\n");
  printf("  ( %lu, %lu, %lu, %lu, %lu, %lu )\n",
         read0,
         read1,
         read2,
         read3,
         read4,
         read5);

  printf("Expected:\n");
  printf("  ( %lu, %lu, %lu, %lu, %lu, %lu )\n",
         VAL0,
         VAL1,
         VAL4,
         VAL5,
         VAL2,
         VAL3);
}
