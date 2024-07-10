#include <cstdio>

#include "cmemoir/cmemoir.h"
#include "cmemoir/test.hpp"

using namespace memoir;

#define KEY0 (uint32_t)1
#define KEY1 (uint32_t)2
#define KEY2 (uint32_t)3
#define INVALID_KEY (uint32_t)1000

#define VAL0 (uint32_t)10
#define VAL1 (uint32_t)20
#define VAL2 (uint32_t)30

int main() {

  TEST(assoc_has) {

    auto map = memoir_allocate_assoc_array(memoir_u32_t, memoir_u32_t);

    memoir_assoc_insert(map, KEY0);
    memoir_assoc_write(u32, VAL0, map, KEY0);
    memoir_assoc_insert(map, KEY1);
    memoir_assoc_write(u32, VAL1, map, KEY1);
    memoir_assoc_insert(map, KEY2);
    memoir_assoc_write(u32, VAL2, map, KEY2);

    EXPECT(memoir_assoc_has(map, KEY0), "KEY0 differs!");
    EXPECT(memoir_assoc_has(map, KEY1), "KEY1 differs!");
    EXPECT(memoir_assoc_has(map, KEY2), "KEY2 differs!");
    EXPECT(not memoir_assoc_has(map, INVALID_KEY), "INVALID_KEY present!");
  }

  TEST(assoc_set) {

    auto map = memoir_allocate_assoc_array(memoir_u32_t, memoir_void_t);

    memoir_assoc_insert(map, KEY0);
    memoir_assoc_insert(map, KEY1);
    memoir_assoc_insert(map, KEY2);

    EXPECT(memoir_assoc_has(map, KEY0), "KEY0 differs!");
    EXPECT(memoir_assoc_has(map, KEY1), "KEY1 differs!");
    EXPECT(memoir_assoc_has(map, KEY2), "KEY2 differs!");
    EXPECT(not memoir_assoc_has(map, INVALID_KEY), "INVALID_KEY present!");
  }

  TEST(assoc_remove) {

    auto map = memoir_allocate_assoc_array(memoir_u32_t, memoir_u32_t);

    memoir_assoc_insert(map, KEY0);
    memoir_assoc_write(u32, VAL0, map, KEY0);
    memoir_assoc_insert(map, KEY1);
    memoir_assoc_write(u32, VAL1, map, KEY1);
    memoir_assoc_insert(map, KEY2);
    memoir_assoc_write(u32, VAL2, map, KEY2);

    memoir_assoc_remove(map, KEY1);

    EXPECT(memoir_assoc_has(map, KEY0), "KEY0 differs!");
    EXPECT(not memoir_assoc_has(map, KEY1), "KEY1 differs!");
    EXPECT(memoir_assoc_has(map, KEY2), "KEY2 differs!");
  }

  TEST(assoc_ref_to_u32) {

    auto objTy = memoir_define_struct_type("Foo", memoir_u64_t);

    auto obj0 = memoir_allocate_struct(objTy);
    auto obj1 = memoir_allocate_struct(objTy);
    auto obj2 = memoir_allocate_struct(objTy);

    memoir_struct_write(u64, 1, obj0, 0);
    memoir_struct_write(u64, 2, obj1, 0);
    memoir_struct_write(u64, 3, obj2, 0);

    auto map = memoir_allocate_assoc_array(memoir_ref_t(objTy), memoir_u64_t);

    memoir_assoc_insert(map, obj0);
    memoir_assoc_write(u64, VAL0, map, obj0);
    memoir_assoc_insert(map, obj1);
    memoir_assoc_write(u64, VAL1, map, obj1);
    memoir_assoc_insert(map, obj2);
    memoir_assoc_write(u64, VAL2, map, obj2);

    auto read0 = memoir_assoc_read(u64, map, obj0);
    auto read1 = memoir_assoc_read(u64, map, obj1);
    auto read2 = memoir_assoc_read(u64, map, obj2);

    EXPECT(read0 == VAL0, "assoc[obj0] differs!");
    EXPECT(read1 == VAL1, "assoc[obj1] differs!");
    EXPECT(read2 == VAL2, "assoc[obj2] differs!");
  }
}
