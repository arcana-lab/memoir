#include <iostream>

#include "cmemoir/cmemoir.h"

using namespace memoir;

#define VAL0 10
#define VAL1 20
#define VAL2 30

auto objTy = memoir_define_struct_type("Foo", memoir_u64_t);

int main() {
  printf("Initializing keys\n");

  auto obj0 = memoir_allocate_struct(objTy);
  auto obj1 = memoir_allocate_struct(objTy);
  auto obj2 = memoir_allocate_struct(objTy);

  memoir_struct_write(u64, 1, obj0, 0);
  memoir_struct_write(u64, 2, obj1, 0);
  memoir_struct_write(u64, 3, obj2, 0);

  printf("Initializing map\n");

  auto map = memoir_allocate_assoc_array(memoir_ref_t(objTy), memoir_u64_t);

  memoir_assoc_insert(map, obj0);
  memoir_assoc_write(u64, VAL0, map, obj0);
  memoir_assoc_insert(map, obj1);
  memoir_assoc_write(u64, VAL1, map, obj1);
  memoir_assoc_insert(map, obj2);
  memoir_assoc_write(u64, VAL2, map, obj2);

  printf("Reading map\n");

  auto read0 = memoir_assoc_read(u64, map, obj0);
  auto read1 = memoir_assoc_read(u64, map, obj1);
  auto read2 = memoir_assoc_read(u64, map, obj2);

  printf(" Result:\n");
  printf("  obj0 -> %d\n", read0);
  printf("  obj1 -> %d\n", read1);
  printf("  obj2 -> %d\n", read2);

  printf("Expected:\n");
  printf("  obj0 -> %d\n", VAL0);
  printf("  obj1 -> %d\n", VAL1);
  printf("  obj2 -> %d\n", VAL2);
}
