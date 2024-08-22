#include <cstdio>

#include "cmemoir/cmemoir.h"
#include "cmemoir/test.hpp"

using namespace memoir;

auto pair_t = memoir_define_struct_type("pair_t", memoir_u32_t, memoir_u32_t);

collection_ref accum_seq(collection_ref accum, size_t i, uint32_t v) {
  auto seq_t = memoir_sequence_type(memoir_u32_t);
  memoir_assert_collection_type(seq_t, accum);
  memoir_return_type(seq_t);

  memoir_seq_insert(u32, v, accum, i);

  return accum;
}

uint32_t accum_closed_seq(uint32_t ignore,
                          size_t i,
                          uint32_t v,
                          collection_ref accum) {
  auto seq_t = memoir_sequence_type(memoir_u32_t);
  memoir_assert_collection_type(seq_t, accum);
  memoir_return_type(seq_t);

  memoir_seq_insert(u32, v, accum, i);

  return ignore;
}

collection_ref filter_seq(collection_ref accum, size_t i, uint32_t v) {
  auto seq_t = memoir_sequence_type(memoir_u32_t);
  memoir_assert_collection_type(seq_t, accum);
  memoir_return_type(seq_t);

  if (v < 10) {
    memoir_seq_insert(u32, v, accum, memoir_end());
  }

  return accum;
}

collection_ref accum_seq_of_tuples(collection_ref accum, size_t i, uint32_t v) {
  auto seq_t = memoir_sequence_type(pair_t);
  memoir_assert_collection_type(seq_t, accum);
  memoir_return_type(seq_t);

  memoir_seq_insert_elem(accum, i);
  memoir_index_write(u32, v, accum, i, 0);
  memoir_index_write(u32, v, accum, i, 1);

  return accum;
}

int main() {

  TEST(accum_seq) {

    auto seq = memoir_allocate_sequence(memoir_u32_t, 100);

    for (size_t i = 0; i < 100; ++i) {
      memoir_index_write(u32, 1, seq, i);
    }

    auto accum = memoir_fold(collection_ref,
                             memoir_allocate_sequence(memoir_u32_t, 0),
                             seq,
                             accum_seq);

    for (size_t i = 0; i < 100; ++i) {
      EXPECT(memoir_index_read(u32, accum, i) == 1, "differs!");
    }
  }

  TEST(filter_seq) {

    auto seq = memoir_allocate_sequence(memoir_u32_t, 100);

    for (size_t i = 0; i < 100; ++i) {
      memoir_index_write(u32, 1, seq, i);
    }

    auto accum = memoir_fold(collection_ref,
                             memoir_allocate_sequence(memoir_u32_t, 0),
                             seq,
                             filter_seq);

    for (size_t i = 0; i < 10; ++i) {
      EXPECT(memoir_index_read(u32, accum, i) == 1, "differs!");
    }
  }

  TEST(accum_closed_seq) {

    auto seq = memoir_allocate_sequence(memoir_u32_t, 100);

    for (size_t i = 0; i < 100; ++i) {
      memoir_index_write(u32, 1, seq, i);
    }

    auto closed = memoir_allocate_sequence(memoir_u32_t, 0);
    memoir_fold(u32, 0, seq, accum_closed_seq, closed);

    for (size_t i = 0; i < 100; ++i) {
      EXPECT(memoir_index_read(u32, closed, i) == 1, "differs!");
    }
  }

  TEST(accum_seq_of_tuples) {

    auto seq = memoir_allocate_sequence(memoir_u32_t, 100);

    for (size_t i = 0; i < 100; ++i) {
      memoir_index_write(u32, 1, seq, i);
    }

    auto accum = memoir_fold(collection_ref,
                             memoir_allocate_sequence(pair_t, 0),
                             seq,
                             accum_seq_of_tuples);

    for (size_t i = 0; i < 100; ++i) {
      EXPECT(memoir_index_read(u32, accum, i, 0) == 1, "differs!");
      EXPECT(memoir_index_read(u32, accum, i, 1) == 1, "differs!");
    }
  }
}
