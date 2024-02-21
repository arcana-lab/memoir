#include <iostream>

#include "cmemoir/cmemoir.h"

using namespace memoir;

auto innerTy = memoir_define_struct_type("Foo", memoir_u32_t, memoir_u32_t);

auto objTy =
    memoir_define_struct_type("Bar", memoir_u32_t, memoir_ref_t(innerTy));

int main() {

  auto innerObj = memoir_allocate_struct(innerTy);

  auto myObj = memoir_allocate_struct(objTy);

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

  printf("Result:   %u, %u, %u\n", read1, read2, read3);
  printf("Expected: 579, 1245, 912\n");
}
