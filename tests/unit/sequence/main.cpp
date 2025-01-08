#include <iostream>

#include "cmemoir/cmemoir.h"

#include "cmemoir/test.hpp"

using namespace memoir;

#define VAL0 uint64_t(10)
#define VAL1 uint64_t(20)
#define VAL2 uint64_t(30)
#define VAL3 uint64_t(40)
#define VAL4 uint64_t(50)
#define VAL5 uint64_t(60)

#define VAL0_0 1
#define VAL0_1 10
#define VAL1_0 2
#define VAL1_1 20
#define VAL2_0 3
#define VAL2_1 30
#define VAL3_0 4
#define VAL3_1 40

auto type = memoir_define_struct_type("Foo", memoir_u32_t, memoir_u32_t);

collection_ref qsort(collection_ref seq, size_t start, size_t end) {
  memoir_assert_collection_type(memoir_sequence_type(memoir_u32_t), seq);
  memoir_return_type(memoir_sequence_type(memoir_u32_t));

  size_t n = end - start;

  // Perform quicksort
  if (end <= start || n <= 1) {
    return seq;
  }

  // Perform insertion sort for n < 3
  if (n == 2) {
    auto left = memoir_index_read(u32, seq, start);
    auto right = memoir_index_read(u32, seq, end - 1);
    if (left > right) {
      memoir_index_write(u32, right, seq, start);
      memoir_index_write(u32, left, seq, end - 1);
    }
    return seq;
  }

  // Select pivot
  auto p = n / 2 + start;

  // Move pivot.
  {
    auto tmp = memoir_index_read(u32, seq, start);
    memoir_index_write(u32, memoir_index_read(u32, seq, p), seq, start);
    memoir_index_write(u32, tmp, seq, p);
  }

  // Get the pivot value.
  auto pv = memoir_index_read(u32, seq, start);

  // Construct partitions.
  auto l = start;
  auto r = end;
  while (true) {
    while (true) {
      r--;
      if (memoir_index_read(u32, seq, r) < pv || r <= l) {
        break;
      }
    }
    while (true) {
      l++;
      if (memoir_index_read(u32, seq, l) > pv || l >= r) {
        break;
      }
    }
    if (l < r) {
      auto tmp = memoir_index_read(u32, seq, l);
      memoir_index_write(u32, memoir_index_read(u32, seq, r), seq, l);
      memoir_index_write(u32, tmp, seq, r);

    } else {
      break;
    }
  }

  // Move the pivot back into place.
  p = r;
  {
    auto tmp = memoir_index_read(u32, seq, start);
    memoir_index_write(u32, memoir_index_read(u32, seq, p), seq, start);
    memoir_index_write(u32, tmp, seq, p);
  }

  // Recurse.
  seq = qsort(seq, start, r);
  seq = qsort(seq, r + 1, end);

  return seq;
}

int main(int argc, char *argv[]) {
  TEST(read_and_write) {
    auto seq = memoir_allocate_sequence(memoir_u64_t, 3);

    memoir_index_write(u64, VAL0, seq, 0);
    memoir_index_write(u64, VAL1, seq, 1);
    memoir_index_write(u64, VAL2, seq, 2);

    auto read0 = memoir_index_read(u64, seq, 0);
    auto read1 = memoir_index_read(u64, seq, 1);
    auto read2 = memoir_index_read(u64, seq, 2);

    EXPECT(read0 == VAL0, "seq[0] differs!");
    EXPECT(read1 == VAL1, "seq[1] differs!");
    EXPECT(read2 == VAL2, "seq[2] differs!");
  }

  TEST(append) {
    auto *seq = memoir_allocate_sequence(memoir_u64_t, 4);

    memoir_index_write(u64, VAL0, seq, 0);
    memoir_index_write(u64, VAL1, seq, 1);
    memoir_index_write(u64, VAL2, seq, 2);
    memoir_index_write(u64, VAL3, seq, 3);

    auto *seq2 = memoir_allocate_sequence(memoir_u64_t, 2);

    memoir_index_write(u64, VAL4, seq2, 0);
    memoir_index_write(u64, VAL5, seq2, 1);

    memoir_seq_append(seq, seq2);

    auto read0 = memoir_index_read(u64, seq, 0);
    auto read1 = memoir_index_read(u64, seq, 1);
    auto read2 = memoir_index_read(u64, seq, 2);
    auto read3 = memoir_index_read(u64, seq, 3);
    auto read4 = memoir_index_read(u64, seq, 4);
    auto read5 = memoir_index_read(u64, seq, 5);

    EXPECT(read0 == VAL0, "seq[0] differs!");
    EXPECT(read1 == VAL1, "seq[1] differs!");
    EXPECT(read2 == VAL2, "seq[2] differs!");
    EXPECT(read3 == VAL3, "seq[3] differs!");
    EXPECT(read4 == VAL4, "seq[4] differs!");
    EXPECT(read5 == VAL5, "seq[5] differs!");
  }

  TEST(if_else) {
    auto *seq = memoir_allocate_sequence(memoir_u64_t, 4);

    memoir_index_write(u64, VAL0, seq, 0);
    memoir_index_write(u64, VAL1, seq, 1);
    memoir_index_write(u64, VAL2, seq, 2);
    memoir_index_write(u64, VAL3, seq, 3);

    auto *seq2 = memoir_allocate_sequence(memoir_u64_t, 2);

    memoir_index_write(u64, VAL4, seq2, 0);
    memoir_index_write(u64, VAL5, seq2, 1);

    if (argc > 1) {
      memoir_insert(seq, 2, memoir_input(seq2));
    }

    if (argc > 1) {
      EXPECT(memoir_index_read(u64, seq, 0) == VAL0, "seq[0] differs!");
      EXPECT(memoir_index_read(u64, seq, 1) == VAL1, "seq[1] differs!");
      EXPECT(memoir_index_read(u64, seq, 2) == VAL4, "seq[2] differs!");
      EXPECT(memoir_index_read(u64, seq, 3) == VAL5, "seq[3] differs!");
      EXPECT(memoir_index_read(u64, seq, 4) == VAL2, "seq[4] differs!");
      EXPECT(memoir_index_read(u64, seq, 5) == VAL3, "seq[5] differs!");
    } else {
      EXPECT(memoir_index_read(u64, seq, 0) == VAL0, "seq[0] differs!");
      EXPECT(memoir_index_read(u64, seq, 1) == VAL1, "seq[1] differs!");
      EXPECT(memoir_index_read(u64, seq, 2) == VAL2, "seq[2] differs!");
      EXPECT(memoir_index_read(u64, seq, 3) == VAL3, "seq[3] differs!");
    }
  }

  TEST(insert) {
    auto *seq = memoir_allocate_sequence(memoir_u64_t, 4);

    memoir_index_write(u64, VAL0, seq, 0);
    memoir_index_write(u64, VAL1, seq, 1);
    memoir_index_write(u64, VAL2, seq, 2);
    memoir_index_write(u64, VAL3, seq, 3);

    auto *seq2 = memoir_allocate_sequence(memoir_u64_t, 2);

    memoir_index_write(u64, VAL4, seq2, 0);
    memoir_index_write(u64, VAL5, seq2, 1);

    memoir_insert(seq, 2, memoir_input(seq2));

    EXPECT(memoir_index_read(u64, seq, 0) == VAL0, "seq[0] differs!");
    EXPECT(memoir_index_read(u64, seq, 1) == VAL1, "seq[1] differs!");
    EXPECT(memoir_index_read(u64, seq, 2) == VAL4, "seq[2] differs!");
    EXPECT(memoir_index_read(u64, seq, 3) == VAL5, "seq[3] differs!");
    EXPECT(memoir_index_read(u64, seq, 4) == VAL2, "seq[4] differs!");
    EXPECT(memoir_index_read(u64, seq, 5) == VAL3, "seq[5] differs!");
  }

  TEST(remove) {
    auto *seq = memoir_allocate_sequence(memoir_u64_t, 6);

    memoir_index_write(u64, VAL0, seq, 0);
    memoir_index_write(u64, VAL1, seq, 1);
    memoir_index_write(u64, VAL2, seq, 2);
    memoir_index_write(u64, VAL3, seq, 3);
    memoir_index_write(u64, VAL4, seq, 4);
    memoir_index_write(u64, VAL5, seq, 5);

    memoir_remove(seq, 3);

    EXPECT(memoir_index_read(u64, seq, 0) == VAL0, "seq[0] differs!");
    EXPECT(memoir_index_read(u64, seq, 1) == VAL1, "seq[1] differs!");
    EXPECT(memoir_index_read(u64, seq, 2) == VAL2, "seq[2] differs!");
    EXPECT(memoir_index_read(u64, seq, 3) == VAL4, "seq[3] differs!");
    EXPECT(memoir_index_read(u64, seq, 4) == VAL5, "seq[4] differs!");
  }

  TEST(sequence_struct) {

    auto seq = memoir_allocate_sequence(type, 3);

    memoir_index_write(u32, VAL0_0, seq, 0, 0);
    memoir_index_write(u32, VAL0_1, seq, 0, 1);

    memoir_index_write(u32, VAL1_0, seq, 1, 0);
    memoir_index_write(u32, VAL1_1, seq, 1, 1);

    memoir_index_write(u32, VAL2_0, seq, 2, 0);
    memoir_index_write(u32, VAL2_1, seq, 2, 1);

    EXPECT(memoir_index_read(u32, seq, 0, 0) == VAL0_0, "[0].0 differs");
    EXPECT(memoir_index_read(u32, seq, 0, 1) == VAL0_1, "[0].1 differs");
    EXPECT(memoir_index_read(u32, seq, 1, 0) == VAL1_0, "[1].0 differs");
    EXPECT(memoir_index_read(u32, seq, 1, 1) == VAL1_1, "[1].1 differs");
    EXPECT(memoir_index_read(u32, seq, 2, 0) == VAL2_0, "[2].0 differs");
    EXPECT(memoir_index_read(u32, seq, 2, 1) == VAL2_1, "[2].1 differs");
  }

  TEST(qsort) {
    auto seq = memoir_allocate_sequence(memoir_u32_t, 100);

    for (uint32_t i = 0; i < 100; ++i) {
      memoir_index_write(u32, i, seq, 100 - i);
    }

    seq = qsort(seq, 0, 100);

    for (uint32_t i = 0; i < 100; ++i) {
      EXPECT(memoir_index_read(u32, seq, i) == i, "Unsorted!");
    }
  }

  TEST(partial_qsort) {
    auto seq = memoir_allocate_sequence(memoir_u32_t, 100);

    for (uint32_t i = 0; i < 100; ++i) {
      memoir_index_write(u32, i, seq, 100 - i);
    }

    seq = qsort(seq, 0, 100);

    for (uint32_t i = 0; i < 10; ++i) {
      EXPECT(memoir_index_read(u32, seq, i) == i, "Unsorted!");
    }
  }

  TEST(boolean) {
    auto seq = memoir_allocate_sequence(memoir_bool_t, 10);

    for (size_t i = 0; i < 10; ++i) {
      memoir_index_write(boolean, (i % 2) == 0, seq, i);
    }

    for (size_t i = 0; i < 10; ++i) {
      EXPECT(memoir_index_read(boolean, seq, i) == ((i % 2) == 0), "differs");
    }
  }

  TEST(insert_struct) {

    auto seq = memoir_allocate_sequence(type, 3);

    memoir_write(u32, VAL0_0, seq, 0, 0);
    memoir_write(u32, VAL0_1, seq, 0, 1);
    memoir_write(u32, VAL1_0, seq, 1, 0);
    memoir_write(u32, VAL1_1, seq, 1, 1);
    memoir_write(u32, VAL2_0, seq, 2, 0);
    memoir_write(u32, VAL2_1, seq, 2, 1);

    memoir_insert(seq, 0);

    memoir_write(u32, VAL3_0, seq, 0, 0);
    memoir_write(u32, VAL3_1, seq, 0, 1);

    EXPECT(memoir_read(u32, seq, 0, 0) == VAL3_0, "[0].0 differs");
    EXPECT(memoir_read(u32, seq, 0, 1) == VAL3_1, "[0].1 differs");
    EXPECT(memoir_read(u32, seq, 1, 0) == VAL0_0, "[1].0 differs");
    EXPECT(memoir_read(u32, seq, 1, 1) == VAL0_1, "[1].1 differs");
    EXPECT(memoir_read(u32, seq, 2, 0) == VAL1_0, "[2].0 differs");
    EXPECT(memoir_read(u32, seq, 2, 1) == VAL1_1, "[2].1 differs");
    EXPECT(memoir_read(u32, seq, 3, 0) == VAL2_0, "[2].0 differs");
    EXPECT(memoir_read(u32, seq, 3, 1) == VAL2_1, "[2].1 differs");
  }
}
