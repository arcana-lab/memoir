#include <iostream>

#include "memoir/c/cmemoir.h"
#include "test.hpp"

using namespace memoir;

auto type = memoir_define_struct_type("Foo", memoir_u32_t, memoir_u32_t);

int main() {
  TEST(fold_assoc_onto_seq) {
    auto *seq = memoir_allocate_sequence(type, 100);

    for (auto i = 0; i < 100; i++) {
      auto *elem = memoir_index_get(struct, seq, i);
      memoir_struct_write(u32, i, elem, 0);
      memoir_struct_write(u32, 100 + i, elem, 1);
    }

    auto *assoc = memoir_allocate_assoc_array(memoir_ref_t(type), memoir_u32_t);
    for (auto i = 99; i >= 0; --i) {
      auto *elem = memoir_index_get(struct, seq, i);
      memoir_assoc_write(u32, i, assoc, elem);
    }

    int sum = 0;
    for (auto i = 0; i < 100; i++) {
      auto *elem = memoir_index_get(struct, seq, i);
      auto read1 = memoir_struct_read(u32, elem, 0);
      auto read2 = memoir_struct_read(u32, elem, 1);
      auto read3 = memoir_assoc_read(u32, assoc, elem);
      sum += read2;
      sum -= read1;
      sum += read3;
    }

    EXPECT(sum == 14950, "sum differs!");
  }

  TEST(fold_assoc_onto_assoc) {
    auto *base = memoir_allocate_assoc_array(memoir_u32_t, type);

    for (auto i = 0; i < 100; i += 2) {
      auto *elem = memoir_assoc_get(struct, base, i);
      memoir_struct_write(u32, i, elem, 0);
      memoir_struct_write(u32, 100 + i, elem, 1);
    }

    auto *assoc = memoir_allocate_assoc_array(memoir_ref_t(type), memoir_u32_t);
    for (auto i = 98; i >= 0; i -= 2) {
      auto *elem = memoir_assoc_get(struct, base, i);
      memoir_assoc_write(u32, i, assoc, elem);
    }

    int sum = 0;
    for (auto i = 0; i < 100; i += 2) {
      auto *elem = memoir_assoc_get(struct, base, i);
      auto read1 = memoir_struct_read(u32, elem, 0);
      auto read2 = memoir_struct_read(u32, elem, 1);
      auto read3 = memoir_assoc_read(u32, assoc, elem);
      sum += read2;
      sum -= read1;
      sum += read3;
    }

    EXPECT(sum == 7450, "sum differs!");
  }

  return 0;
}
