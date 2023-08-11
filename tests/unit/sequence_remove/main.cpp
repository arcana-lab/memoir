#include <iostream>

#include "cmemoir.h"

using namespace memoir;

#define VAL0 10
#define VAL1 20
#define VAL2 30
#define VAL3 40
#define VAL4 50
#define VAL5 60

int main() {
  auto *seq = memoir_allocate_sequence(memoir_u64_t, 6);

  memoir_index_write(u64, VAL0, seq, 0);
  memoir_index_write(u64, VAL1, seq, 1);
  memoir_index_write(u64, VAL2, seq, 2);
  memoir_index_write(u64, VAL3, seq, 3);
  memoir_index_write(u64, VAL4, seq, 4);
  memoir_index_write(u64, VAL5, seq, 5);

  auto read0 = memoir_index_read(u64, seq, 0);
  auto read1 = memoir_index_read(u64, seq, 1);
  auto read2 = memoir_index_read(u64, seq, 2);
  auto read3 = memoir_index_read(u64, seq, 3);
  auto read4 = memoir_index_read(u64, seq, 4);
  auto read5 = memoir_index_read(u64, seq, 5);

  std::cout << "Result:\n";
  std::cout << " Sequence 1:\n";
  std::cout << "  HEAD -> " << std::to_string(read0) << "\n";
  std::cout << "       -> " << std::to_string(read1) << "\n";
  std::cout << "       -> " << std::to_string(read2) << "\n\n";
  std::cout << " Sequence 2:\n";
  std::cout << " HEAD  -> " << std::to_string(read3) << "\n";
  std::cout << "       -> " << std::to_string(read4) << "\n";
  std::cout << "       -> " << std::to_string(read5) << "\n\n";

  std::cout << "Expected:\n";
  std::cout << " Sequence 1:\n";
  std::cout << "  HEAD -> " << std::to_string(VAL0) << "\n";
  std::cout << "       -> " << std::to_string(VAL1) << "\n";
  std::cout << "       -> " << std::to_string(VAL2) << "\n\n";
  std::cout << " Sequence 2:\n";
  std::cout << "  HEAD -> " << std::to_string(VAL3) << "\n";
  std::cout << "       -> " << std::to_string(VAL4) << "\n";
  std::cout << "       -> " << std::to_string(VAL5) << "\n\n";

  memoir_seq_remove(seq, 3);

  read0 = memoir_index_read(u64, seq, 0);
  read1 = memoir_index_read(u64, seq, 1);
  read2 = memoir_index_read(u64, seq, 2);
  read3 = memoir_index_read(u64, seq, 3);
  read4 = memoir_index_read(u64, seq, 4);

  printf(" Result:\n");
  printf("  HEAD -> %d\n", read0);
  printf("       -> %d\n", read1);
  printf("       -> %d\n", read2);
  printf("       -> %d\n", read3);
  printf("       -> %d\n\n", read4);

  printf("Expected:\n");
  printf("  HEAD -> %d\n", VAL0);
  printf("       -> %d\n", VAL1);
  printf("       -> %d\n", VAL2);
  printf("       -> %d\n", VAL4);
  printf("       -> %d\n\n", VAL5);
}
