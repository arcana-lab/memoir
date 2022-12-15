#include <iostream>

#include "memoir.h"

using namespace memoir;

#define VAL0 10
#define VAL1 20
#define VAL2 30

int main() {
  std::cout << "\nInitializing sequences\n";

  auto seq = memoir_allocate_sequence(memoir_u64_t, 3);

  memoir_index_write(u64, VAL0, seq, 0);
  memoir_index_write(u64, VAL1, seq, 1);
  memoir_index_write(u64, VAL2, seq, 2);

  std::cout << "\nReading sequence\n";

  auto read0 = memoir_index_read(u64, seq, 0);
  auto read1 = memoir_index_read(u64, seq, 1);
  auto read2 = memoir_index_read(u64, seq, 2);

  std::cout << "\nSlicing sequence\n";

  auto slice01 = memoir_sequence_slice(seq, 0, 2);
  auto slice12 = memoir_sequence_slice(seq, 1, -1);

  std::cout << "\nReading slices\n";

  auto read00 = memoir_index_read(u64, slice01, 0);
  std::cerr << std::to_string(read00) << "\n";
  auto read01 = memoir_index_read(u64, slice01, 1);
  std::cerr << std::to_string(read01) << "\n";
  auto read10 = memoir_index_read(u64, slice12, 0);
  std::cerr << std::to_string(read10) << "\n";
  auto read11 = memoir_index_read(u64, slice12, 1);
  std::cerr << std::to_string(read11) << "\n";

  std::cout << "Result:\n";
  std::cout << " Slice 1:\n";
  std::cout << "  HEAD -> " << std::to_string(read00) << "\n";
  std::cout << "       -> " << std::to_string(read01) << "\n";
  std::cout << " Slice 2:\n";
  std::cout << "  HEAD -> " << std::to_string(read10) << "\n";
  std::cout << "       -> " << std::to_string(read11) << "\n\n";

  std::cout << "Expected:\n";
  std::cout << "Slice 1:\n";
  std::cout << "  HEAD -> " << std::to_string(VAL0) << "\n";
  std::cout << "       -> " << std::to_string(VAL1) << "\n";
  std::cout << "Slice 2:\n";
  std::cout << "  HEAD -> " << std::to_string(VAL1) << "\n";
  std::cout << "       -> " << std::to_string(VAL2) << "\n\n";
}
