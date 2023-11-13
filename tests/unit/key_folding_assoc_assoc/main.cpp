#include <iostream>

#include "cmemoir/cmemoir.h"

using namespace memoir;

// Second argument is dead.
auto type = memoir_define_struct_type("Foo", memoir_u64_t, memoir_u64_t);

int main() {
  auto *base = memoir_allocate_assoc_array(memoir_u64_t, type);

  for (auto i = 0; i < 100; i += 2) {
    auto *elem = memoir_assoc_get(struct, base, i);
    memoir_struct_write(u64, i, elem, 0);
    memoir_struct_write(u64, 100 + i, elem, 1);
  }

  auto *assoc = memoir_allocate_assoc_array(memoir_ref_t(type), memoir_u64_t);
  for (auto i = 98; i >= 0; i -= 2) {
    auto *elem = memoir_assoc_get(struct, base, i);
    memoir_assoc_write(u64, i, assoc, elem);
  }

  int sum = 0;
  for (auto i = 0; i < 100; i += 2) {
    auto *elem = memoir_assoc_get(struct, base, i);
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
