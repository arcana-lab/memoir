#include <iostream>

#include "memoir.h"

using namespace memoir;

Type *objTy = MEMOIR_FUNC(define_struct_type)("Foo",
                                              3,
                                              MEMOIR_FUNC(u64_type)(),
                                              MEMOIR_FUNC(u64_type)(),
                                              MEMOIR_FUNC(u64_type)());

int main() {
  Object *myObj = MEMOIR_FUNC(allocate_struct)(objTy);

  MEMOIR_FUNC(write_u64)(123, myObj, 1);
  MEMOIR_FUNC(write_u64)(456, myObj, 2);
  MEMOIR_FUNC(write_u64)(789, myObj, 3);

  uint64_t read1 = MEMOIR_FUNC(read_u64)(myObj, 0);
  uint64_t read2 = MEMOIR_FUNC(read_u64)(myObj, 1);
  uint64_t read3 = MEMOIR_FUNC(read_u64)(myObj, 2);

  std::cerr << "1: " << read1 << "\n";
  std::cerr << "2: " << read2 << "\n";
  std::cerr << "3: " << read3 << "\n\n";

  MEMOIR_FUNC(write_u64)(read1 + read2, myObj, 0);
  MEMOIR_FUNC(write_u64)(read2 + read3, myObj, 1);
  MEMOIR_FUNC(write_u64)(read3 + read1, myObj, 2);

  read1 = MEMOIR_FUNC(read_u64)(myObj, 0);
  read2 = MEMOIR_FUNC(read_u64)(myObj, 1);
  read3 = MEMOIR_FUNC(read_u64)(myObj, 2);

  std::cerr << "1: " << read1 << "\n";
  std::cerr << "2: " << read2 << "\n";
  std::cerr << "3: " << read3 << "\n\n";
}
