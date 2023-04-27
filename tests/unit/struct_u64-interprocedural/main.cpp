#include <iostream>

#include "cmemoir.h"

using namespace memoir;

auto objTy =
    memoir_define_struct_type("Foo", memoir_u64_t);

void write(Struct* myObj)
{
  memoir_struct_write(u64, 123, myObj, 0);
}

int main() {
  auto myObj = memoir_allocate_struct(objTy);
  write(myObj);
  auto read1 = memoir_struct_read(u64, myObj, 0);
  std::cerr << "1: " << read1 << "\n";
  return 0;
}

