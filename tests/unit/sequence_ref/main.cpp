#include <iostream>

#include "cmemoir/cmemoir.h"

using namespace memoir;

#define VAL0_0 1
#define VAL0_1 10
#define VAL1_0 2
#define VAL1_1 20
#define VAL2_0 3
#define VAL2_1 30

auto type = memoir_define_struct_type("Foo", memoir_u32_t, memoir_u32_t);

int main() {

  auto obj0 = memoir_allocate_struct(type);
  memoir_struct_write(u32, rand(), obj0, 0);
  memoir_struct_write(u32, VAL0_1, obj0, 1);

  auto obj1 = memoir_allocate_struct(type);
  memoir_struct_write(u32, rand(), obj1, 0);
  memoir_struct_write(u32, VAL1_1, obj1, 1);

  auto obj2 = memoir_allocate_struct(type);
  memoir_struct_write(u32, rand(), obj2, 0);
  memoir_struct_write(u32, VAL2_1, obj2, 1);

  auto seq = memoir_allocate_sequence(memoir_ref_t(type), 3);
  memoir_index_write(struct_ref, obj0, seq, 0);
  memoir_index_write(struct_ref, obj1, seq, 1);
  memoir_index_write(struct_ref, obj2, seq, 2);

  obj0 = memoir_index_read(struct_ref, seq, 0);
  memoir_struct_write(u32, VAL0_0, obj0, 0);
  obj1 = memoir_index_read(struct_ref, seq, 1);
  memoir_struct_write(u32, VAL1_0, obj1, 0);
  obj2 = memoir_index_read(struct_ref, seq, 2);
  memoir_struct_write(u32, VAL2_0, obj2, 0);

  obj0 = memoir_index_read(struct_ref, seq, 0);
  auto read0_0 = memoir_struct_read(u32, obj0, 0);
  auto read0_1 = memoir_struct_read(u32, obj0, 1);
  obj1 = memoir_index_read(struct_ref, seq, 1);
  auto read1_0 = memoir_struct_read(u32, obj1, 0);
  auto read1_1 = memoir_struct_read(u32, obj1, 1);
  obj2 = memoir_index_read(struct_ref, seq, 2);
  auto read2_0 = memoir_struct_read(u32, obj2, 0);
  auto read2_1 = memoir_struct_read(u32, obj2, 1);

  printf(" Result:\n");
  printf("  [ (%d, %d), (%d, %d), (%d, %d) ]\n",
         read0_0,
         read0_1,
         read1_0,
         read1_1,
         read2_0,
         read2_1);

  printf("Expected:\n");
  printf("  [ (%d, %d), (%d, %d), (%d, %d) ]\n",
         VAL0_0,
         VAL0_1,
         VAL1_0,
         VAL1_1,
         VAL2_0,
         VAL2_1);
}
