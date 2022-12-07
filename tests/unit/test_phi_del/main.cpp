#include <iostream>

#include "memoir.h"

using namespace memoir;

Type *objTy =
    MEMOIR_FUNC(define_struct_type)("Foo", 1, MEMOIR_FUNC(u64_type)());

int main() {
  Object *myObj1 = MEMOIR_FUNC(allocate_struct)(objTy);
  Object *myObj2 = MEMOIR_FUNC(allocate_struct)(objTy);
  Object *myObj3; // = allocateStruct(objTy);

  MEMOIR_FUNC(write_u64)(123, myObj1, 0);
  MEMOIR_FUNC(write_u64)(456, myObj2, 0);

  if (MEMOIR_FUNC(read_u64)(myObj1, 0) == 0) {
    myObj3 = myObj2;
  } else {
    myObj3 = myObj1;
  }

  MEMOIR_FUNC(delete_object)(myObj3);
}
