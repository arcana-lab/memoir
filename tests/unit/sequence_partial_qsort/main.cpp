#include <iostream>

#include "cmemoir.h"

using namespace memoir;

#define K 5

memoir::Collection *qsort(memoir::Collection *seq_to_sort, size_t n) {
  // Type information
  memoir_assert_collection_type(memoir_sequence_type(memoir_u64_t),
                                seq_to_sort);

  // Perform quicksort
  if (n < 2) {
    return seq_to_sort;
  }

  // Perform insertion sort for n < 3
  if (n < 3) {
    auto l_slice = memoir_sequence_slice(seq_to_sort, 0, 0);
    auto r_slice = memoir_sequence_slice(seq_to_sort, 1, 1);
    if (memoir_index_read(u64, l_slice, 0)
        > memoir_index_read(u64, r_slice, 0)) {
      return memoir_join(r_slice, l_slice);
    } else {
      return memoir_join(l_slice, r_slice);
    }
  }

  // Select pivot
  auto p = n / 2;
  auto p_slice = memoir_sequence_slice(seq_to_sort, p, p);
  auto l_slice = memoir_sequence_slice(seq_to_sort, 0, p - 1);
  auto r_slice = memoir_sequence_slice(seq_to_sort, p + 1, -1);
  auto rest = memoir_join(l_slice, r_slice);
  auto remaining = n - 1;

  // Create the initial partitions
  auto l_part = memoir_allocate_sequence(memoir_u64_t, 0);
  auto l_size = 0;
  auto r_part = memoir_allocate_sequence(memoir_u64_t, 0);
  auto r_size = 0;

  do {
    auto first = memoir_sequence_slice(rest, 0, 0);
    if (remaining > 1) {
      rest = memoir_sequence_slice(rest, 1, -1);
    }
    remaining -= 1;

    if (memoir_index_read(u64, first, 0) < memoir_index_read(u64, p_slice, 0)) {
      l_part = memoir_join(l_part, first);
      l_size += 1;
    } else {
      r_part = memoir_join(r_part, first);
      r_size += 1;
    }

  } while (remaining > 0);

  std::cerr << "sorting left\n";
  auto l_sorted = qsort(l_part, l_size);
  std::cerr << "sorting right\n";
  auto r_sorted = qsort(r_part, r_size);
  std::cerr << "joining\n";

  return memoir_join(l_sorted, p_slice, r_sorted);
}

int main(int argc, char *argv[]) {
  std::cout << "\nInitializing sequence\n";

  if (argc <= 1) {
    return 0;
  }

  auto length = argc - 1;

  auto seq = memoir_allocate_sequence(memoir_u64_t, length);

  for (auto i = 0; i < length; i++) {
    auto input_element = atoi(argv[1 + i]);
    memoir_index_write(u64, input_element, seq, i);
  }

  std::cout << "\nSorting sequence\n";

  auto sorted_seq = qsort(seq, length);

  auto print_length = (length < K) ? length : K;
  std::cout << "\nResult (first " << print_length << " elements): \n";
  for (auto i = 0; i < print_length; i++) {
    auto read = memoir_index_read(u64, sorted_seq, i);
    if (i == 0) {
      std::cout << std::to_string(read);
    } else {
      std::cout << ", " << std::to_string(read);
    }
  }
  std::cout << "\n";
}
