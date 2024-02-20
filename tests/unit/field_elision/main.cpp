#include <iostream>

#include "cmemoir/cmemoir.h"

using namespace memoir;

auto other_type = memoir_define_struct_type("Other",
                                            memoir_u64_t,
                                            memoir_u64_t,
                                            memoir_u64_t);

auto objTy =
    memoir_define_struct_type("Foo", other_type, memoir_u64_t, memoir_u64_t);

// type escapes, should not be elided.
// auto escaped_type =
//     memoir_define_struct_type("Esc", memoir_u64_t, memoir_u64_t,
//     memoir_u64_t);

// extern void escape_hatch(Struct *);

void recurse(memoir::Struct *obj) {
  memoir_assert_struct_type(objTy, obj);

  auto read = memoir_struct_read(u64, obj, 2);
  if (read > 700) {
    memoir_struct_write(u64, read - 1, obj, 2);
    recurse(obj);
  }

  return;
}

void dump(memoir::Struct *obj) {
  memoir_assert_struct_type(objTy, obj);

  auto inner = memoir_struct_get(struct, obj, 0);
  auto read11 = memoir_struct_read(u64, inner, 0);
  auto read12 = memoir_struct_read(u64, inner, 1);
  auto read13 = memoir_struct_read(u64, inner, 2);
  auto read2 = memoir_struct_read(u64, obj, 1);
  auto read3 = memoir_struct_read(u64, obj, 2);

  printf(".1.1 = %lu\n", read11);
  printf(".1.2 = %lu\n", read12);
  printf(".1.3 = %lu\n", read13);
  printf(".2   = %lu\n", read2);
  printf(".3   = %lu\n\n", read3);
}

int main() {
  auto *obj = memoir_allocate_struct(objTy);

  auto *inner = memoir_struct_get(struct, obj, 0);
  memoir_struct_write(u64, 1, inner, 0);
  memoir_struct_write(u64, 22, inner, 1);
  memoir_struct_write(u64, 333, inner, 2);
  memoir_struct_write(u64, 456, obj, 1);
  memoir_struct_write(u64, 789, obj, 2);

  recurse(obj);

  dump(obj);

  // Uncomment the following lines to test the escape analysis.
  // auto escapee = memoir_allocate_struct(escaped_type);
  // memoir_struct_write(u64, 123, escapee, 1);
  // memoir_struct_write(u64, 123, escapee, 2);
  // escape_hatch(escapee);

  return 0;
}
