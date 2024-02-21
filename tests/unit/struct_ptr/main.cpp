#include <iostream>

#include "cmemoir/cmemoir.h"

using namespace memoir;

struct c_struct_t {
  uint64_t a;
  uint64_t b;
};

auto foo_ty = memoir_define_struct_type("Foo", memoir_ptr_t);

int main() {

  auto foo = memoir_allocate_struct(foo_ty);

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

  printf("Result:\n");
  printf("  a: %lu\n", c_ptr->a);
  printf("  b: %lu\n", c_ptr->b);

  printf("Expected:\n");
  printf("  a: %lu\n", uint64_t(456));
  printf("  b: %lu\n", uint64_t(2));
}
