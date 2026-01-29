#include <cstdio>

#include "memoir/c/cmemoir.h"
#include "test.hpp"

using namespace memoir;

uint32_t sum_seq(uint32_t accum, size_t i, uint32_t v) {
  return accum + v;
}

uint32_t sum_inner_seq(uint32_t accum, size_t i, uint32_t v) {
  return accum + v;
}

uint32_t sum_outer_seq(uint32_t accum, size_t i, collection_ref v) {
  memoir_assert_collection_type(memoir_sequence_type(memoir_u32_t), v);

  return memoir_fold(u32, sum_inner_seq, accum, v);
}

collection_ref accum_seq(collection_ref accum, size_t i, uint32_t v) {
  auto seq_t = memoir_sequence_type(memoir_u32_t);
  memoir_assert_collection_type(seq_t, accum);
  memoir_return_type(seq_t);

  memoir_index_write(u32, v, accum, i);

  return accum;
}

uint32_t sum_seq_times(uint32_t accum, size_t i, uint32_t v, uint32_t x) {
  return accum + v * x;
}

uint32_t sum_seq_times_mut(uint32_t accum, size_t i, uint32_t v, uint32_t *x) {
  return accum + v * ((*x)++);
}

uint32_t sum_seq_mut_set(uint32_t accum,
                         size_t i,
                         uint32_t v,
                         collection_ref seen) {
  memoir_assert_collection_type(memoir_assoc_type(memoir_u32_t, memoir_void_t),
                                seen);

  if (memoir_has(seen, v)) {
    return accum;
  } else {
    memoir_insert(seen, v);
  }

  return accum + v;
}

uint32_t sum_assoc(uint32_t accum, uint32_t k, uint32_t v) {
  return accum + k + v;
}

uint32_t sum_set(uint32_t accum, uint32_t k) {
  return accum + k;
}

collection_ref accum_hist_seq(collection_ref accum, size_t i, uint32_t v) {
  auto assoc_t = memoir_assoc_type(memoir_u32_t, memoir_u32_t);
  memoir_assert_collection_type(assoc_t, accum);
  memoir_return_type(assoc_t);

  if (not memoir_has(accum, v)) {
    memoir_insert(accum, v);
    memoir_write(u32, 1, accum, v);
  } else {
    auto old = memoir_read(u32, accum, v);
    memoir_write(u32, 1 + old, accum, v);
  }

  return accum;
}

collection_ref push_each(collection_ref accum, size_t i, uint32_t v) {
  auto seq_t = memoir_sequence_type(memoir_u32_t);
  memoir_assert_collection_type(seq_t, accum);
  memoir_return_type(seq_t);

  memoir_insert_value(v, accum, memoir_end());

  return accum;
}

int main() {

  TEST(fold_seq) {

    auto seq = memoir_allocate_sequence(memoir_u32_t, 100);

    for (size_t i = 0; i < 100; ++i) {
      memoir_index_write(u32, 1, seq, i);
    }

    auto sum = memoir_fold(u32, sum_seq, 0, seq);

    EXPECT(sum == 100, "Sum incorrect!");
  }

  TEST(fold_assoc) {
    auto assoc = memoir_allocate_assoc(memoir_u32_t, memoir_u32_t);

    memoir_insert(assoc, 10);
    memoir_write(u32, 1, assoc, 10);
    memoir_insert(assoc, 20);
    memoir_write(u32, 2, assoc, 20);
    memoir_insert(assoc, 30);
    memoir_write(u32, 3, assoc, 30);

    auto sum = memoir_fold(u32, sum_assoc, 0, assoc);

    EXPECT(sum == (10 + 1 + 2 + 20 + 3 + 30), "Sum incorrect!");
  }

  TEST(fold_set) {
    auto set = memoir_allocate_assoc(memoir_u32_t, memoir_void_t);

    memoir_insert(set, 10);
    memoir_insert(set, 20);
    memoir_insert(set, 30);

    auto sum = memoir_fold(u32, sum_set, 0, set);

    EXPECT(sum == (10 + 20 + 30), "Sum incorrect!");
  }

  TEST(accum_seq) {

    auto seq = memoir_allocate_sequence(memoir_u32_t, 100);

    for (size_t i = 0; i < 100; ++i) {
      memoir_index_write(u32, 1, seq, i);
    }

    auto accum = memoir_fold(ref,
                             accum_seq,
                             memoir_allocate_sequence(memoir_u32_t, 100),
                             seq);

    for (size_t i = 0; i < 100; ++i) {
      EXPECT(memoir_index_read(u32, accum, i) == 1,
             "accumulated sequence differs!");
    }
  }

  TEST(accum_assoc) {
    auto seq = memoir_allocate_sequence(memoir_u32_t, 100);

    for (size_t i = 0; i < 100; ++i) {
      memoir_index_write(u32, 1, seq, i);
    }

    // Compute the histogram of the sequence.
    auto hist = memoir_fold(ref,
                            accum_hist_seq,
                            memoir_allocate_assoc(memoir_u32_t, memoir_u32_t),
                            seq);

    // The result should be { 1 : 100 }
    EXPECT(memoir_size(hist) == 1, "Too many elements!");
    EXPECT(memoir_read(u32, hist, 1) == 100, "Incorrect count for key=1");
  }

  TEST(close_immut_scalar) {
    auto seq = memoir_allocate_sequence(memoir_u32_t, 100);

    for (size_t i = 0; i < 100; ++i) {
      memoir_index_write(u32, 1, seq, i);
    }

    const uint32_t x = 10;

    auto sum = memoir_fold(u32, sum_seq_times, 0, seq, memoir_closed(x));

    EXPECT(sum == 1000, "Sum incorrect!");
  }

  TEST(close_mut_scalar) {
    auto seq = memoir_allocate_sequence(memoir_u32_t, 10);

    for (size_t i = 0; i < 10; ++i) {
      memoir_write(u32, 1, seq, i);
    }

    uint32_t x;
    x = 1;

    auto sum = memoir_fold(u32, sum_seq_times_mut, 0, seq, memoir_closed(&x));

    EXPECT(sum == (1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10), "Sum incorrect!");
    EXPECT(x == 11, "x incorrect!");
  }

  TEST(close_mut_collection) {
    auto seq = memoir_allocate_sequence(memoir_u32_t, 10);

    for (size_t i = 0; i < 9; ++i) {
      memoir_index_write(u32, i % 5, seq, i);
    }
    memoir_index_write(u32, 5, seq, 9);

    auto set = memoir_allocate_assoc(memoir_u32_t, memoir_void_t);

    auto sum = memoir_fold(u32, sum_seq_mut_set, 0, seq, memoir_closed(set));

    EXPECT(sum == (0 + 1 + 2 + 3 + 4 + 5), "Sum incorrect!");
    EXPECT(memoir_size(set) == 6, "Size incorrect!");
  }

  TEST(rfold_seq) {

    auto seq = memoir_allocate_sequence(memoir_u32_t, 10);

    for (size_t i = 0; i < 10; ++i) {
      memoir_write(u32, 9 - i, seq, i);
    }

    auto rev = memoir_rfold(ref,
                            push_each,
                            memoir_allocate_sequence(memoir_u32_t, 0),
                            seq);

    EXPECT(memoir_size(rev) == 10, "wrong size!");
    for (size_t i = 0; i < 10; ++i) {
      EXPECT(memoir_read(u32, rev, i) == i, "differs!");
    }
  }

  TEST(empty) {
    auto seq = memoir_allocate_sequence(memoir_u32_t, 0);

    auto sum = memoir_fold(u32, sum_seq, 0, seq);

    EXPECT(sum == 0, "differs!");
  }

  TEST(nested) {
#define N 10

    auto obj = memoir_allocate(
        memoir_sequence_type(memoir_sequence_type(memoir_u32_t)),
        N,
        N);

    for (auto i = 0; i < N; ++i) {
      for (auto j = 0; j < N; ++j) {
        memoir_write(u32, i + j, obj, i, j);
      }
    }

    EXPECT(N == memoir_size(obj), "outer size differs");
    for (auto i = 0; i < N; ++i) {
      EXPECT(N == memoir_size(obj, i), "inner size differs");
      EXPECT((N * i + N * (N - 1) / 2) == memoir_fold(u32, sum_seq, 0, obj, i),
             "inner fold differs!");
    }

    auto nested_fold = memoir_fold(u32, sum_outer_seq, 0, obj);
    EXPECT(((N - 1) * N * N) == nested_fold, "nested fold differs!");

#undef N
  }
}
