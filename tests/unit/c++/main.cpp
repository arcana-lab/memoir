#include <cmemoir/test.hpp>

#include <memoir++/sequence.hh>

using namespace memoir;

void increment(Ref<Seq<int>> seq_ref) {
  Seq<int> seq(seq_ref);

  for (auto i = 0; i < seq.size(); ++i) {
    seq[i] = seq[i] + 1;
  }
}

void recurse(Ref<Seq<int>> seq_ref, size_t i) {
  Seq<int> seq(seq_ref);

  if (i >= seq.size()) {
    return;
  }

  seq[i] = seq[i] + 1;

  recurse(&seq, i + 1);
}

int main() {

  TEST(init_write_read) {
    size_t n = 10;

    Seq<int> seq(n);

    for (auto i = 0; i < n; ++i) {
      seq[i] = i;
    }

    int sum = 0;
    for (auto i = 0; i < n; ++i) {
      sum += seq[i];
      EXPECT(seq[i] == i, "seq[i] differs");
    }

    EXPECT(sum == (0 + 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9), "incorrect sum");
  }

  TEST(for_loop) {
    size_t n = 10;

    Seq<int> seq(n);

    for (auto i = 0; i < n; ++i) {
      seq[i] = i;
    }

    int sum = 0;
    for (auto v : seq) {
      sum += v;
    }

    EXPECT(sum == (0 + 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9), "incorrect sum");
  }

  TEST(call) {
    size_t n = 10;

    Seq<int> seq(n);

    for (auto i = 0; i < n; ++i) {
      seq[i] = i;
    }

    increment(&seq);

    int sum = 0;
    for (auto v : seq) {
      sum += v;
    }

    EXPECT(sum == (1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10), "incorrect sum");
  }

  TEST(call_recursive) {
    size_t n = 10;

    Seq<int> seq(n);

    for (auto i = 0; i < n; ++i) {
      seq[i] = i;
    }

    recurse(&seq, 0);

    int sum = 0;
    for (auto v : seq) {
      sum += v;
    }

    EXPECT(sum == (1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10), "incorrect sum");
  }

  return 0;
}
