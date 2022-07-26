#include <iostream>

#include "memoir.h"

using namespace memoir;

Type *objTy = defineStructType("Foo", 1, UInt64Type());

int main() {
  Object *myObj1 = allocateStruct(objTy);
  Object *myObj2 = allocateStruct(objTy);
  Object *myObj3; // = allocateStruct(objTy);

  Field *field1 = getStructField(myObj1, 0);
  Field *field2 = getStructField(myObj2, 0);

  writeUInt64(field1, 123);
  writeUInt64(field2, 456);

  if (readUInt64(field1) == 0) {
    myObj3 = myObj2;
  } else {
    myObj3 = myObj1;
  }
  deleteObject(myObj3);
}
