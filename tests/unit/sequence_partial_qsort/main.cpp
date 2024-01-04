#include <iostream>

#include "cmemoir/cmemoir.h"

using namespace memoir;

#define B 1

void qsort(memoir::Collection *seq, size_t start, size_t end) {
  size_t n = end - start;
  // Perform quicksort
  if (end <= start || n <= 1) {
    return;
  }

  // Perform insertion sort for n < 3
  if (n == 2) {
    if (memoir_index_read(u64, seq, start)
        > memoir_index_read(u64, seq, end - 1)) {
      memoir_seq_swap_within(seq, start, end - 1);
    }
    return;
  }

  // Select pivot
  auto p = n / 2 + start;

  // Move pivot.
  memoir_seq_swap_within(seq, start, p);

  // Get the pivot value.
  auto pv = memoir_index_read(u64, seq, start);

  // Construct partitions.
  auto l = start;
  auto r = end;
  while (true) {
    while (true) {
      r--;
      if (memoir_index_read(u64, seq, r) < pv || r <= l) {
        break;
      }
    }
    while (true) {
      l++;
      if (memoir_index_read(u64, seq, l) > pv || l >= r) {
        break;
      }
    }
    if (l < r) {
      memoir_seq_swap_within(seq, l, r);
    } else {
      break;
    }
  }

  // Move the pivot back into place.
  p = r;
  memoir_seq_swap_within(seq, start, p);

  // Recurse.
  qsort(seq, start, r);
  qsort(seq, r + 1, end);

  return;
}

int main(int argc, char *argv[]) {
  printf("\nInitializing sequence\n");

  if (argc <= 1) {
    return 0;
  }

  auto size = argc - 1;
  auto seq = memoir_allocate_sequence(memoir_u64_t, size);

  for (auto i = 1; i < argc; i++) {
    auto input_element = atoi(argv[i]);
    memoir_index_write(u64, input_element, seq, i - 1);
  }

  printf("\nSorting sequence\n");

  qsort(seq, 0, size);

  auto print_length = size > B ? B : size;

  printf("\nResult: \n");
  for (auto i = 0; i < print_length; i++) {
    auto read = memoir_index_read(u64, seq, i);
    printf("%lu, ", read);
  }
  printf("\n");

  return 0;
}
