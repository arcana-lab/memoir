#include <iostream>

#include "memoir.h"

using namespace memoir;

Type *innerTy = memoir_define_struct_type("Foo", memoir_u64_t, memoir_u64_t);

Type *objTy =
    memoir_define_struct_type("Bar", memoir_u64_t, memoir_ref_type(innerTy));

int main() {

  Object *innerObj = memoir_allocate_struct(innerTy);

  Object *myObj = memoir_allocate_struct(objTy);

  // Update the outer fields
  memoir_write_u64(123, myObj, 0);
  memoir_write_ref(innerObj, myObj, 1);

  // Get the object from the pointer
  Object *deref = memoir_read_ref(myObj, 1);

  // Update the inner fields
  memoir_write_u64(456, deref, 0);
  memoir_write_u64(789, deref, 1);

  uint64_t read1 = memoir_read_u64(myObj, 0);
  uint64_t read2 = memoir_read_u64(deref, 0);
  uint64_t read3 = memoir_read_u64(deref, 1);

  memoir_write_u64(read1 + read2, myObj, 0);
  memoir_write_u64(read2 + read3, deref, 0);
  memoir_write_u64(read3 + read1, deref, 1);

  read1 = memoir_read_u64(myObj, 0);
  read2 = memoir_read_u64(deref, 0);
  read3 = memoir_read_u64(deref, 1);

  std::cout << "read1: " << read1 << "\n";
  std::cout << "read2: " << read2 << "\n";
  std::cout << "read3: " << read3 << "\n";
}
