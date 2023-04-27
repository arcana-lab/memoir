#include <iostream>

#include "cmemoir.h"

using namespace memoir;

auto objTy =
    memoir_define_struct_type("Foo", memoir_u64_t);

int64_t readstruct(Struct* myObj)
{
  memoir_assert_struct_type(objTy, myObj);
  return memoir_struct_read(u64, myObj, 0);
}

int main() {
  auto myObj = memoir_allocate_struct(objTy);
  memoir_struct_write(u64, 123, myObj, 0);
  int64_t read1 = readstruct(myObj);
  // // auto read1 = memoir_struct_read(u64, myObj, 0);
  std::cerr << "1: " << "\n";
  return read1;
}

