#include <iostream>

#include "cmemoir.h"

using namespace memoir;

auto objTy =
    memoir_define_struct_type("Foo", memoir_u64_t, memoir_u64_t, memoir_u64_t);

int main() {
  auto myObj = memoir_allocate_struct(objTy);

  memoir_struct_write(u64, 123, myObj, 0);
  memoir_struct_write(u64, 456, myObj, 1);
  memoir_struct_write(u64, 789, myObj, 2);

  auto read1 = memoir_struct_read(u64, myObj, 0);
  auto read2 = memoir_struct_read(u64, myObj, 1);
  auto read3 = memoir_struct_read(u64, myObj, 2);

  std::cerr << "1: " << read1 << "\n";
  std::cerr << "2: " << read2 << "\n";
  std::cerr << "3: " << read3 << "\n\n";

  memoir_struct_write(u64, read1 + read2, myObj, 0);
  memoir_struct_write(u64, read2 + read3, myObj, 1);
  memoir_struct_write(u64, read3 + read1, myObj, 2);

  read1 = memoir_struct_read(u64, myObj, 0);
  read2 = memoir_struct_read(u64, myObj, 1);
  read3 = memoir_struct_read(u64, myObj, 2);

  std::cerr << "1: " << read1 << "\n";
  std::cerr << "2: " << read2 << "\n";
  std::cerr << "3: " << read3 << "\n\n";

  return 0;
}
