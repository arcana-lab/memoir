#include <iostream>

#include "cmemoir/cmemoir.h"

using namespace memoir;

// Second argument is dead.
auto objTy =
    memoir_define_struct_type("Foo", memoir_u64_t, memoir_u64_t, memoir_u64_t);

// type escapes, should not be freed.
auto escaped_type =
    memoir_define_struct_type("Esc", memoir_u64_t, memoir_u64_t, memoir_u64_t);

extern void escape_hatch(Struct *);

int main() {
  auto myObj = memoir_allocate_struct(objTy);

  memoir_struct_write(u64, 123, myObj, 0);
  memoir_struct_write(u64, 789, myObj, 2);

  auto read1 = memoir_struct_read(u64, myObj, 0);
  auto read3 = memoir_struct_read(u64, myObj, 2);

  printf(".1= %lu\n", read1);
  printf(".3= %lu\n\n", read3);

  memoir_struct_write(u64, read1 + read3, myObj, 0);
  memoir_struct_write(u64, read3 - read1, myObj, 2);

  read1 = memoir_struct_read(u64, myObj, 0);
  read3 = memoir_struct_read(u64, myObj, 2);

  printf(".1= %lu\n", read1);
  printf(".3= %lu\n\n", read3);

  auto escapee = memoir_allocate_struct(escaped_type);
  memoir_struct_write(u64, 123, escapee, 1);
  memoir_struct_write(u64, 123, escapee, 2);
  escape_hatch(escapee);

  return 0;
}
