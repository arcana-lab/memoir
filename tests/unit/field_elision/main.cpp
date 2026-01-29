#include <iostream>

#include "memoir/c/cmemoir.h"
#include "test.hpp"

using namespace memoir;

auto other_type = memoir_define_struct_type("Other",
                                            memoir_u32_t,
                                            memoir_u32_t,
                                            memoir_u32_t);

auto objTy =
    memoir_define_struct_type("Foo", other_type, memoir_u32_t, memoir_u32_t);

void recurse(memoir::Struct *obj) {
  memoir_assert_struct_type(objTy, obj);

  auto read = memoir_struct_read(u32, obj, 2);
  if (read > 700) {
    memoir_struct_write(u32, read - 1, obj, 2);
    recurse(obj);
  }

  return;
}

int main() {

  TEST(elide_field) {
    auto *obj = memoir_allocate_struct(objTy);

    auto *inner = memoir_struct_get(struct, obj, 0);
    memoir_struct_write(u32, 1, inner, 0);
    memoir_struct_write(u32, 22, inner, 1);
    memoir_struct_write(u32, 333, inner, 2);
    memoir_struct_write(u32, 456, obj, 1);
    memoir_struct_write(u32, 789, obj, 2);

    recurse(obj);

    inner = memoir_struct_get(struct, obj, 0);
    auto read11 = memoir_struct_read(u32, inner, 0);
    auto read12 = memoir_struct_read(u32, inner, 1);
    auto read13 = memoir_struct_read(u32, inner, 2);
    auto read2 = memoir_struct_read(u32, obj, 1);
    auto read3 = memoir_struct_read(u32, obj, 2);

    EXPECT(read11 == 1, ".1.1 differs");
    EXPECT(read12 == 22, ".1.2 differs");
    EXPECT(read13 == 333, ".1.3 differs");
    EXPECT(read2 == 456, ".2 differs");
    EXPECT(read3 == 700, ".2 differs");
  }

  return 0;
}
