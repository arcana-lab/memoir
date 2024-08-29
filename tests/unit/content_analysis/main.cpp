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

collection_ref flip(collection_ref accum, size_t i, struct_ref v) {
  memoir_assert_struct_type(pair_t, v);

  auto seq_t = memoir_sequence_type(pair_t);
  memoir_assert_collection_type(seq_t, accum);
  memoir_return_type(seq_t);

  memoir_seq_insert_elem(accum, i);
  memoir_index_write(u32, memoir_struct_read(u32, v, 1), accum, i, 0);
  memoir_index_write(u32, memoir_struct_read(u32, v, 0), accum, i, 1);

  return accum;
}

template <unsigned F>
collection_ref zip(collection_ref accum, size_t i, uint32_t v) {
  auto seq_t = memoir_sequence_type(pair_t);
  memoir_assert_collection_type(seq_t, accum);
  memoir_return_type(seq_t);

  memoir_index_write(u32, v, accum, i, F);

  return accum;
}

auto data_t = memoir_define_struct_type("data_t", memoir_u32_t);

auto node_t = memoir_define_struct_type("node_t", memoir_u32_t, memoir_u32_t);

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

  TEST(flip) {

    auto seq = memoir_allocate_sequence(pair_t, 100);

    for (size_t i = 0; i < 100; ++i) {
      memoir_index_write(u32, 1, seq, i, 0);
      memoir_index_write(u32, 2, seq, i, 1);
    }

    auto accum = memoir_fold(collection_ref,
                             memoir_allocate_sequence(pair_t, 0),
                             seq,
                             flip);

    for (size_t i = 0; i < 100; ++i) {
      EXPECT(memoir_index_read(u32, accum, i, 0) == 2, "differs!");
      EXPECT(memoir_index_read(u32, accum, i, 1) == 1, "differs!");
    }
  }

  TEST(zip) {

    auto seq1 = memoir_allocate_sequence(memoir_u32_t, 100);
    auto seq2 = memoir_allocate_sequence(memoir_u32_t, 100);

    for (size_t i = 0; i < 100; ++i) {
      memoir_index_write(u32, 1, seq1, i);
      memoir_index_write(u32, 2, seq2, i);
    }

    auto accum = memoir_allocate_sequence(pair_t, 100);
    accum = memoir_fold(collection_ref, accum, seq1, zip<0>);
    accum = memoir_fold(collection_ref, accum, seq2, zip<1>);

    for (size_t i = 0; i < 100; ++i) {
      EXPECT(memoir_index_read(u32, accum, i, 0) == 1, "differs!");
      EXPECT(memoir_index_read(u32, accum, i, 1) == 2, "differs!");
    }
  }

  TEST(while_loop) {
    auto seq = memoir_allocate_sequence(memoir_u32_t, 100);

    // Initialize a ring.
    for (size_t i = 0; i < 100; ++i) {
      memoir_index_write(u32, (i + 1) % 100, seq, i);
    }

    // Iterate over the sequence using a while loop, stopping when we've hit the
    // root again.
    auto hist = memoir_allocate_assoc_array(memoir_u32_t, memoir_u32_t);
    uint32_t i = 0;
    do {
      if (not memoir_assoc_has(hist, i)) {
        memoir_assoc_insert(hist, i);
        memoir_assoc_write(u32, 1, hist, i);
      } else {
        memoir_assoc_write(u32, 1 + memoir_assoc_read(u32, hist, i), hist, i);
      }

      i = memoir_index_read(u32, seq, i);
    } while (i != 0);
  }

  TEST(while_loop2) {
    // Initialize a ring.
    auto ring = memoir_allocate_assoc_array(memoir_u32_t, memoir_u32_t);
    for (uint32_t i = 0; i < 100; ++i) {
      memoir_assoc_insert(ring, i);
      memoir_assoc_write(u32, (i + 1) % 100, ring, i);
    }

    // Iterate over the sequence using a while loop, stopping when we've hit the
    // root again.
    auto hist = memoir_allocate_assoc_array(memoir_u32_t, memoir_u32_t);
    uint32_t i = memoir_assoc_read(u32, ring, 0);
    while (i != 0) {
      if (not memoir_assoc_has(hist, i)) {
        memoir_assoc_insert(hist, i);
        memoir_assoc_write(u32, 1, hist, i);
      } else {
        memoir_assoc_write(u32, 1 + memoir_assoc_read(u32, hist, i), hist, i);
      }

      i = memoir_assoc_read(u32, ring, i);
    }

    EXPECT(not memoir_assoc_has(hist, 0), "shouldn't have visitied 0");
    for (auto i = 1; i < 100; ++i) {
      EXPECT(memoir_assoc_read(u32, hist, i) == 1, "!= 1");
    }
  }
}
