#include <iostream>

#include "cmemoir.h"

using namespace memoir;

#define VAL0 10
#define VAL1 20
#define VAL2 30
#define VAL3 40
#define VAL4 50
#define VAL5 60
#define VAL6 70
#define VAL7 80
#define VAL8 90

int main() {
  std::cout << "\nInitializing sequences\n";

  auto seq0 = memoir_allocate_sequence(memoir_u64_t, 3);

  memoir_index_write(u64, VAL0, seq0, 0);
  memoir_index_write(u64, VAL1, seq0, 1);
  memoir_index_write(u64, VAL2, seq0, 2);

  auto seq1 = memoir_allocate_sequence(memoir_u64_t, 3);

  memoir_index_write(u64, VAL3, seq1, 0);
  memoir_index_write(u64, VAL4, seq1, 1);
  memoir_index_write(u64, VAL5, seq1, 2);

  auto seq2 = memoir_allocate_sequence(memoir_u64_t, 3);

  memoir_index_write(u64, VAL6, seq2, 0);
  memoir_index_write(u64, VAL7, seq2, 1);
  memoir_index_write(u64, VAL8, seq2, 2);

  std::cout << "\nReading sequences\n";

  auto read00 = memoir_index_read(u64, seq0, 0);
  auto read01 = memoir_index_read(u64, seq0, 1);
  auto read02 = memoir_index_read(u64, seq0, 2);
  auto read10 = memoir_index_read(u64, seq1, 0);
  auto read11 = memoir_index_read(u64, seq1, 1);
  auto read12 = memoir_index_read(u64, seq1, 2);
  auto read20 = memoir_index_read(u64, seq2, 0);
  auto read21 = memoir_index_read(u64, seq2, 1);
  auto read22 = memoir_index_read(u64, seq2, 2);

  std::cout << "Result:\n";
  std::cout << " Sequence 1:\n";
  std::cout << "  HEAD -> " << std::to_string(read00) << "\n";
  std::cout << "       -> " << std::to_string(read01) << "\n";
  std::cout << "       -> " << std::to_string(read02) << "\n\n";
  std::cout << " Sequence 2:\n";
  std::cout << " HEAD  -> " << std::to_string(read10) << "\n";
  std::cout << "       -> " << std::to_string(read11) << "\n";
  std::cout << "       -> " << std::to_string(read12) << "\n\n";
  std::cout << " Sequence 3:\n";
  std::cout << " HEAD  -> " << std::to_string(read20) << "\n";
  std::cout << "       -> " << std::to_string(read21) << "\n";
  std::cout << "       -> " << std::to_string(read22) << "\n\n";

  std::cout << "Expected:\n";
  std::cout << " Sequence 1:\n";
  std::cout << "  HEAD -> " << std::to_string(VAL0) << "\n";
  std::cout << "       -> " << std::to_string(VAL1) << "\n";
  std::cout << "       -> " << std::to_string(VAL2) << "\n\n";
  std::cout << " Sequence 2:\n";
  std::cout << "  HEAD -> " << std::to_string(VAL3) << "\n";
  std::cout << "       -> " << std::to_string(VAL4) << "\n";
  std::cout << "       -> " << std::to_string(VAL5) << "\n\n";
  std::cout << " Sequence 3:\n";
  std::cout << "  HEAD -> " << std::to_string(VAL6) << "\n";
  std::cout << "       -> " << std::to_string(VAL7) << "\n";
  std::cout << "       -> " << std::to_string(VAL8) << "\n\n";

  std::cout << "\nJoining sequences\n";

  auto join_seq = memoir_join(seq0, seq1, seq2);

  std::cout << "\nReading sequence\n";

  auto read0 = memoir_index_read(u64, join_seq, 0);
  auto read1 = memoir_index_read(u64, join_seq, 1);
  auto read2 = memoir_index_read(u64, join_seq, 2);
  auto read3 = memoir_index_read(u64, join_seq, 3);
  auto read4 = memoir_index_read(u64, join_seq, 4);
  auto read5 = memoir_index_read(u64, join_seq, 5);
  auto read6 = memoir_index_read(u64, join_seq, 6);
  auto read7 = memoir_index_read(u64, join_seq, 7);
  auto read8 = memoir_index_read(u64, join_seq, 8);

  std::cout << " Result:\n";
  std::cout << "  HEAD -> " << std::to_string(read0) << "\n";
  std::cout << "       -> " << std::to_string(read1) << "\n";
  std::cout << "       -> " << std::to_string(read2) << "\n";
  std::cout << "       -> " << std::to_string(read3) << "\n";
  std::cout << "       -> " << std::to_string(read4) << "\n";
  std::cout << "       -> " << std::to_string(read5) << "\n";
  std::cout << "       -> " << std::to_string(read6) << "\n";
  std::cout << "       -> " << std::to_string(read7) << "\n";
  std::cout << "       -> " << std::to_string(read8) << "\n\n";

  std::cout << "Expected:\n";
  std::cout << "  HEAD -> " << std::to_string(VAL0) << "\n";
  std::cout << "       -> " << std::to_string(VAL1) << "\n";
  std::cout << "       -> " << std::to_string(VAL2) << "\n";
  std::cout << "       -> " << std::to_string(VAL3) << "\n";
  std::cout << "       -> " << std::to_string(VAL4) << "\n";
  std::cout << "       -> " << std::to_string(VAL5) << "\n";
  std::cout << "       -> " << std::to_string(VAL6) << "\n";
  std::cout << "       -> " << std::to_string(VAL7) << "\n";
  std::cout << "       -> " << std::to_string(VAL8) << "\n\n";
}