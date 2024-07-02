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

auto type = memoir_define_struct_type("Foo", memoir_u32_t, memoir_u32_t);

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
      memoir_seq_insert_range(seq2, seq, 2);
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

    memoir_seq_insert_range(seq2, seq, 2);

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

    memoir_seq_remove(seq, 3);

    EXPECT(memoir_index_read(u64, seq, 0) == VAL0, "seq[0] differs!");
    EXPECT(memoir_index_read(u64, seq, 1) == VAL1, "seq[1] differs!");
    EXPECT(memoir_index_read(u64, seq, 2) == VAL2, "seq[2] differs!");
    EXPECT(memoir_index_read(u64, seq, 3) == VAL4, "seq[3] differs!");
    EXPECT(memoir_index_read(u64, seq, 4) == VAL5, "seq[4] differs!");
  }

  TEST(swap) {
    auto *seq = memoir_allocate_sequence(memoir_u64_t, 4);

    memoir_index_write(u64, VAL0, seq, 0);
    memoir_index_write(u64, VAL1, seq, 1);
    memoir_index_write(u64, VAL2, seq, 2);
    memoir_index_write(u64, VAL3, seq, 3);

    memoir_seq_swap(seq, 0, seq, memoir_size(seq) - 1);

    EXPECT(memoir_index_read(u64, seq, 0) == VAL3, "seq[0] differs!");
    EXPECT(memoir_index_read(u64, seq, 1) == VAL1, "seq[1] differs!");
    EXPECT(memoir_index_read(u64, seq, 2) == VAL2, "seq[2] differs!");
    EXPECT(memoir_index_read(u64, seq, 3) == VAL0, "seq[3] differs!");
  }

  TEST(sequence_ref) {
    auto *obj0 = memoir_allocate_struct(type);
    memoir_struct_write(u32, rand(), obj0, 0);
    memoir_struct_write(u32, VAL0_1, obj0, 1);

    auto *obj1 = memoir_allocate_struct(type);
    memoir_struct_write(u32, rand(), obj1, 0);
    memoir_struct_write(u32, VAL1_1, obj1, 1);

    auto *obj2 = memoir_allocate_struct(type);
    memoir_struct_write(u32, rand(), obj2, 0);
    memoir_struct_write(u32, VAL2_1, obj2, 1);

    auto seq = memoir_allocate_sequence(memoir_ref_t(type), 3);
    memoir_index_write(struct_ref, obj0, seq, 0);
    memoir_index_write(struct_ref, obj1, seq, 1);
    memoir_index_write(struct_ref, obj2, seq, 2);

    obj0 = memoir_index_read(struct_ref, seq, 0);
    memoir_struct_write(u32, VAL0_0, obj0, 0);
    obj1 = memoir_index_read(struct_ref, seq, 1);
    memoir_struct_write(u32, VAL1_0, obj1, 0);
    obj2 = memoir_index_read(struct_ref, seq, 2);
    memoir_struct_write(u32, VAL2_0, obj2, 0);

    obj0 = memoir_index_read(struct_ref, seq, 0);
    EXPECT(memoir_struct_read(u32, obj0, 0) == VAL0_0, "[0].0 differs");
    EXPECT(memoir_struct_read(u32, obj0, 1) == VAL0_1, "[0].1 differs");
    obj1 = memoir_index_read(struct_ref, seq, 1);
    EXPECT(memoir_struct_read(u32, obj1, 0) == VAL1_0, "[1].0 differs");
    EXPECT(memoir_struct_read(u32, obj1, 1) == VAL1_1, "[1].1 differs");
    obj2 = memoir_index_read(struct_ref, seq, 2);
    EXPECT(memoir_struct_read(u32, obj2, 0) == VAL2_0, "[2].0 differs");
    EXPECT(memoir_struct_read(u32, obj2, 1) == VAL2_1, "[2].1 differs");
  }
}
