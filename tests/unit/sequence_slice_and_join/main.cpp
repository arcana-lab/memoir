#include <iostream>

#include "cmemoir/cmemoir.h"

using namespace memoir;

#define VAL0 10
#define VAL1 20
#define VAL2 30
#define VAL3 40
#define VAL4 50
#define VAL5 60

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

  std::cout << "\nReading sequences\n";

  auto read00 = memoir_index_read(u64, seq0, 0);
  auto read01 = memoir_index_read(u64, seq0, 1);
  auto read02 = memoir_index_read(u64, seq0, 2);
  auto read10 = memoir_index_read(u64, seq1, 0);
  auto read11 = memoir_index_read(u64, seq1, 1);
  auto read12 = memoir_index_read(u64, seq1, 2);

  std::cout << "Result:\n";
  std::cout << " Sequence 1:\n";
  std::cout << "  HEAD -> " << std::to_string(read00) << "\n";
  std::cout << "       -> " << std::to_string(read01) << "\n";
  std::cout << "       -> " << std::to_string(read02) << "\n\n";
  std::cout << " Sequence 2:\n";
  std::cout << " HEAD  -> " << std::to_string(read10) << "\n";
  std::cout << "       -> " << std::to_string(read11) << "\n";
  std::cout << "       -> " << std::to_string(read12) << "\n\n";

  std::cout << "Expected:\n";
  std::cout << " Sequence 1:\n";
  std::cout << "  HEAD -> " << std::to_string(VAL0) << "\n";
  std::cout << "       -> " << std::to_string(VAL1) << "\n";
  std::cout << "       -> " << std::to_string(VAL2) << "\n\n";
  std::cout << " Sequence 2:\n";
  std::cout << "  HEAD -> " << std::to_string(VAL3) << "\n";
  std::cout << "       -> " << std::to_string(VAL4) << "\n";
  std::cout << "       -> " << std::to_string(VAL5) << "\n\n";

  std::cout << "\nSlicing sequences\n";

  auto slice_seq_01 = memoir_sequence_slice(seq0, 0, 1);
  auto slice_seq_22 = memoir_sequence_slice(seq0, 2, 2);
  auto slice_seq_33 = memoir_sequence_slice(seq1, 0, 0);
  auto slice_seq_45 = memoir_sequence_slice(seq1, 1, 2);

  std::cout << "\nJoining center slices\n";

  auto join_seq_23 = memoir_join(slice_seq_22, slice_seq_33);

  std::cout << "\nReading sequences\n";

  auto read_s00 = memoir_index_read(u64, slice_seq_01, 0);
  auto read_s01 = memoir_index_read(u64, slice_seq_01, 1);
  auto read_s10 = memoir_index_read(u64, join_seq_23, 0);
  auto read_s11 = memoir_index_read(u64, join_seq_23, 1);
  auto read_s20 = memoir_index_read(u64, slice_seq_45, 0);
  auto read_s21 = memoir_index_read(u64, slice_seq_45, 1);

  std::cout << " Result:\n";
  std::cout << "  HEAD -> " << std::to_string(read_s00) << "\n";
  std::cout << "       -> " << std::to_string(read_s01) << "\n\n";
  std::cout << "  HEAD -> " << std::to_string(read_s10) << "\n";
  std::cout << "       -> " << std::to_string(read_s11) << "\n\n";
  std::cout << "  HEAD -> " << std::to_string(read_s20) << "\n";
  std::cout << "       -> " << std::to_string(read_s21) << "\n\n";

  std::cout << "Expected:\n";
  std::cout << "  HEAD -> " << std::to_string(VAL0) << "\n";
  std::cout << "       -> " << std::to_string(VAL1) << "\n\n";
  std::cout << "  HEAD -> " << std::to_string(VAL2) << "\n";
  std::cout << "       -> " << std::to_string(VAL3) << "\n\n";
  std::cout << "  HEAD -> " << std::to_string(VAL4) << "\n";
  std::cout << "       -> " << std::to_string(VAL5) << "\n\n";

  std::cout << "\nJoining all slices\n";

  auto join_seq = memoir_join(slice_seq_01, join_seq_23, slice_seq_45);

  std::cout << "\nReading sequences\n";

  auto read0 = memoir_index_read(u64, join_seq, 0);
  auto read1 = memoir_index_read(u64, join_seq, 1);
  auto read2 = memoir_index_read(u64, join_seq, 2);
  auto read3 = memoir_index_read(u64, join_seq, 3);
  auto read4 = memoir_index_read(u64, join_seq, 4);
  auto read5 = memoir_index_read(u64, join_seq, 5);

  std::cout << " Result:\n";
  std::cout << "  HEAD -> " << std::to_string(read0) << "\n";
  std::cout << "       -> " << std::to_string(read1) << "\n";
  std::cout << "       -> " << std::to_string(read2) << "\n";
  std::cout << "       -> " << std::to_string(read3) << "\n";
  std::cout << "       -> " << std::to_string(read4) << "\n";
  std::cout << "       -> " << std::to_string(read5) << "\n\n";

  std::cout << "Expected:\n";
  std::cout << "  HEAD -> " << std::to_string(VAL0) << "\n";
  std::cout << "       -> " << std::to_string(VAL1) << "\n";
  std::cout << "       -> " << std::to_string(VAL2) << "\n";
  std::cout << "       -> " << std::to_string(VAL3) << "\n";
  std::cout << "       -> " << std::to_string(VAL4) << "\n";
  std::cout << "       -> " << std::to_string(VAL5) << "\n\n";
}
