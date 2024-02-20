#include <iostream>

#include "cmemoir/cmemoir.h"

using namespace memoir;

// Second argument is dead.
auto type = memoir_define_struct_type("Foo", memoir_u32_t, memoir_u32_t);

int main() {
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

  printf("sum = %lu\n", sum);

  return 0;
}
