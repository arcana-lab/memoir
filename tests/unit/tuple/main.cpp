#include <iostream>

#include "memoir/c/cmemoir.h"
#include "test.hpp"

using namespace memoir;

struct c_t {
  uint64_t a;
  uint64_t b;
};

int main() {

  TEST(struct_u32) {
    auto myObj = memoir_allocate(
        memoir_tuple_type(memoir_u32_t, memoir_u32_t, memoir_u32_t));

    memoir_write(u32, 123, myObj, 0);
    memoir_write(u32, 456, myObj, 1);
    memoir_write(u32, 789, myObj, 2);

    auto read1 = memoir_read(u32, myObj, 0);
    auto read2 = memoir_read(u32, myObj, 1);
    auto read3 = memoir_read(u32, myObj, 2);

    memoir_write(u32, read1 + read2, myObj, 0);
    memoir_write(u32, read2 + read3, myObj, 1);
    memoir_write(u32, read3 + read1, myObj, 2);

    read1 = memoir_read(u32, myObj, 0);
    read2 = memoir_read(u32, myObj, 1);
    read3 = memoir_read(u32, myObj, 2);

    EXPECT(read1 == 579, "obj.0 differs!");
    EXPECT(read2 == 1245, "obj.1 differs!");
    EXPECT(read3 == 912, "obj.2 differs!");
  }

  TEST(struct_ptr) {
    auto foo = memoir_allocate(memoir_tuple_type(memoir_ptr_t));

    // Update the inner fields
    c_t c_obj;
    c_obj.a = 1;
    c_obj.b = 2;

    // Update the outer fields
    memoir_write(ptr, (char *)&c_obj, foo, 0);

    // Get the object from the pointer
    c_t *c_ptr = (c_t *)memoir_read(ptr, foo, 0);

    // Update the inner fields
    c_ptr->a = 456;

    EXPECT(c_ptr->a == 456, "ptr->a differs!");
    EXPECT(c_ptr->b == 2, "ptr->b differs!");
  }

  TEST(struct_ref) {
    auto inner_t = memoir_tuple_type(memoir_u32_t, memoir_u32_t);
    auto outer_t = memoir_tuple_type(memoir_u32_t, inner_t);
    auto obj = memoir_allocate(outer_t);

    // Update the outer fields
    memoir_write(u32, 123, obj, 0);

    // Update the inner fields
    memoir_write(u32, 456, obj, 1, 0);
    memoir_write(u32, 789, obj, 1, 1);

    auto read1 = memoir_read(u32, obj, 0);
    auto read2 = memoir_read(u32, obj, 1, 0);
    auto read3 = memoir_read(u32, obj, 1, 1);

    memoir_write(u32, read1 + read2, obj, 0);
    memoir_write(u32, read2 + read3, obj, 1, 0);
    memoir_write(u32, read3 + read1, obj, 1, 1);

    read1 = memoir_read(u32, obj, 0);
    read2 = memoir_read(u32, obj, 1, 0);
    read3 = memoir_read(u32, obj, 1, 1);

    EXPECT(read1 == 579, "obj.0 differs!");
    EXPECT(read2 == 1245, "obj.0.0 differs!");
    EXPECT(read3 == 912, "obj.0.1 differs!");
  }
}
