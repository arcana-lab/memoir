#include <iostream>

#include "cmemoir/cmemoir.h"
#include "cmemoir/test.hpp"

using namespace memoir;

struct c_struct_t {
  uint64_t a;
  uint64_t b;
};

int main() {

  TEST(struct_u32) {
    auto myObj =
        memoir_allocate_struct(memoir_define_struct_type("struct_u32_t",
                                                         memoir_u32_t,
                                                         memoir_u32_t,
                                                         memoir_u32_t));

    memoir_struct_write(u32, 123, myObj, 0);
    memoir_struct_write(u32, 456, myObj, 1);
    memoir_struct_write(u32, 789, myObj, 2);

    auto read1 = memoir_struct_read(u32, myObj, 0);
    auto read2 = memoir_struct_read(u32, myObj, 1);
    auto read3 = memoir_struct_read(u32, myObj, 2);

    memoir_struct_write(u32, read1 + read2, myObj, 0);
    memoir_struct_write(u32, read2 + read3, myObj, 1);
    memoir_struct_write(u32, read3 + read1, myObj, 2);

    read1 = memoir_struct_read(u32, myObj, 0);
    read2 = memoir_struct_read(u32, myObj, 1);
    read3 = memoir_struct_read(u32, myObj, 2);

    EXPECT(read1 == 579, "obj.0 differs!");
    EXPECT(read2 == 1245, "obj.1 differs!");
    EXPECT(read3 == 912, "obj.2 differs!");
  }

  TEST(struct_ptr) {
    auto foo = memoir_allocate_struct(
        memoir_define_struct_type("struct_ptr_t", memoir_ptr_t));

    // Update the inner fields
    c_struct_t c_obj;
    c_obj.a = 1;
    c_obj.b = 2;

    // Update the outer fields
    memoir_struct_write(ptr, &c_obj, foo, 0);

    // Get the object from the pointer
    c_struct_t *c_ptr = (c_struct_t *)memoir_struct_read(ptr, foo, 0);

    // Update the inner fields
    c_ptr->a = 456;

    EXPECT(c_ptr->a == 456, "ptr->a differs!");
    EXPECT(c_ptr->b == 2, "ptr->b differs!");
  }

  TEST(struct_ref) {
    auto innerObj =
        memoir_allocate_struct(memoir_define_struct_type("struct_ref_inner_t",
                                                         memoir_u32_t,
                                                         memoir_u32_t));

    auto myObj = memoir_allocate_struct(memoir_define_struct_type(
        "struct_ref_outer_t",
        memoir_u32_t,
        memoir_ref_t(memoir_struct_type("struct_ref_inner_t"))));

    // Update the outer fields
    memoir_struct_write(u32, 123, myObj, 0);
    memoir_struct_write(struct_ref, innerObj, myObj, 1);

    // Get the object from the pointer
    auto deref = memoir_struct_read(struct_ref, myObj, 1);

    // Update the inner fields
    memoir_struct_write(u32, 456, deref, 0);
    memoir_struct_write(u32, 789, deref, 1);

    auto read1 = memoir_struct_read(u32, myObj, 0);
    auto read2 = memoir_struct_read(u32, deref, 0);
    auto read3 = memoir_struct_read(u32, deref, 1);

    memoir_struct_write(u32, read1 + read2, myObj, 0);
    memoir_struct_write(u32, read2 + read3, deref, 0);
    memoir_struct_write(u32, read3 + read1, deref, 1);

    read1 = memoir_struct_read(u32, myObj, 0);
    read2 = memoir_struct_read(u32, deref, 0);
    read3 = memoir_struct_read(u32, deref, 1);

    EXPECT(read1 == 579, "obj.0 differs!");
    EXPECT(read2 == 1245, "obj.0.0 differs!");
    EXPECT(read3 == 912, "obj.0.1 differs!");
  }
}
