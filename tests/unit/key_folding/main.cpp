#include <iostream>

#include "cmemoir.h"

using namespace memoir;

// Second argument is dead.
auto type = memoir_define_struct_type("Foo", memoir_u64_t, memoir_u64_t);

int main() {
  auto *seq = memoir_allocate_sequence(type, 100);

  for (auto i = 0; i < 100; i++) {
    auto *elem = memoir_index_get(struct, seq, i);
    memoir_struct_write(u64, i, elem, 0);
    memoir_struct_write(u64, 100 + i, elem, 1);
  }

  auto *assoc = memoir_allocate_assoc_array(memoir_ref_t(type), memoir_u64_t);
  for (auto i = 99; i >= 0; --i) {
    auto *elem = memoir_index_get(struct, seq, i);
    memoir_assoc_write(u64, i, assoc, elem);
  }

  int sum = 0;
  for (auto i = 0; i < 100; i++) {
    auto *elem = memoir_index_get(struct, seq, i);
    auto read1 = memoir_struct_read(u64, elem, 0);
    auto read2 = memoir_struct_read(u64, elem, 1);
    auto read3 = memoir_assoc_read(u64, assoc, elem);
    sum += read2;
    sum -= read1;
    sum += read3;
  }

  printf("sum = %lu\n", sum);

  return 0;
}
