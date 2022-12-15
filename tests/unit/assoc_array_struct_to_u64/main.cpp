#include <iostream>

#include "cmemoir.h"

using namespace memoir;

#define VAL0 10
#define VAL1 20
#define VAL2 30

auto objTy = memoir_define_struct_type("Foo", memoir_u64_t);

int main() {
  std::cout << "Initializing keys\n";

  auto obj0 = memoir_allocate_struct(objTy);
  auto obj1 = memoir_allocate_struct(objTy);
  auto obj2 = memoir_allocate_struct(objTy);

  memoir_struct_write(u64, 1, obj0, 0);
  memoir_struct_write(u64, 2, obj1, 0);
  memoir_struct_write(u64, 3, obj2, 0);

  std::cout << "Initializing map\n";

  auto map = memoir_allocate_assoc_array(objTy, memoir_u64_t);

  memoir_assoc_write(u64, VAL0, map, obj0);
  memoir_assoc_write(u64, VAL1, map, obj1);
  memoir_assoc_write(u64, VAL2, map, obj2);

  std::cout << "Reading map\n";

  auto read0 = memoir_assoc_read(u64, map, obj0);
  auto read1 = memoir_assoc_read(u64, map, obj1);
  auto read2 = memoir_assoc_read(u64, map, obj2);

  std::cout << " Result:\n";
  std::cout << "  obj0 -> " << std::to_string(read0) << "\n";
  std::cout << "  obj1 -> " << std::to_string(read1) << "\n";
  std::cout << "  obj2 -> " << std::to_string(read2) << "\n";

  std::cout << "Expected:\n";
  std::cout << "  obj0 -> " << std::to_string(VAL0) << "\n";
  std::cout << "  obj1 -> " << std::to_string(VAL1) << "\n";
  std::cout << "  obj2 -> " << std::to_string(VAL2) << "\n";
}
