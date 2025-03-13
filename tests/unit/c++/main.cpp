#include <cmemoir/test.hpp>

#include <memoir++/assoc.hh>
#include <memoir++/sequence.hh>

using namespace memoir;

AUTO_STRUCT(foo, FIELD(int, x), FIELD(int, y));

void increment_seq(Ref<Seq<int>> seq_ref) {
  Seq<int> seq(seq_ref);

  for (auto i = 0; i < seq.size(); ++i) {
    seq[i] = seq[i] + 1;
  }
}

void increment_assoc(Ref<Assoc<int, float>> map_ref) {
  Assoc<int, float> map(map_ref);

  for (auto i = 0; i < map.size(); ++i) {
    map[i] = map[i] + 1.0;
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

  size_t n = 10;

  TEST(seq_init_write_read) {
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

  TEST(seq_for_loop) {

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

#if 0
  TEST(seq_call) {

    Seq<int> seq(n);

    for (auto i = 0; i < n; ++i) {
      seq[i] = i;
    }

    increment_seq(&seq);

    int sum = 0;
    for (auto v : seq) {
      sum += v;
    }

    EXPECT(sum == (1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10), "incorrect sum");
  }
#endif

#if 0
  TEST(seq_call_recursive) {

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
#endif

#if 0
  TEST(seq_nested_object) {
    Seq<foo> seq(n);

    for (auto i = 0; i < n; ++i) {
      seq[i].x = i;
      seq[i].y = 2 * i;
    }

    int sum_x = 0;
    int sum_y = 0;
    for (auto i = 0; i < n; ++i) {
      sum_x += seq[i].x;
      sum_y += seq[i].y;
    }

    EXPECT(sum_x == (0 + 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9), "incorrect sum_x");
    EXPECT(sum_y == (2 * (0 + 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9)),
           "incorrect sum_y");
  }
#endif

#if 0
  TEST(assoc_init_write_read) {
    Assoc<int, float> map;

    for (auto i = 0; i < n; ++i) {
      map[i] = float(i) + 0.5;
    }

    float sum = 0.0;
    for (auto i = 0; i < n; ++i) {
      sum += map[i];
      EXPECT(map[i] == (float(i) + 0.5), "map[i] differs");
    }

    EXPECT(sum == (0.5 + 1.5 + 2.5 + 3.5 + 4.5 + 5.5 + 6.5 + 7.5 + 8.5 + 9.5),
           "incorrect sum");
  }
#endif

#if 0  
  TEST(assoc_call) {
    Assoc<int, float> map;

    for (auto i = 0; i < n; ++i) {
      map[i] = float(i) + 0.5;
    }

    increment_assoc(&map);

    float sum = 0.0;
    for (auto i = 0; i < n; ++i) {
      sum += map[i];
      EXPECT(map[i] == (float(i) + 1.5), "map[i] differs");
    }

    EXPECT(sum == (1.5 + 2.5 + 3.5 + 4.5 + 5.5 + 6.5 + 7.5 + 8.5 + 9.5 + 10.5),
           "incorrect sum");
  }
#endif

  return 0;
}
