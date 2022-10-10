#include <iostream>

#include "memoir.h"

using namespace memoir;

Type *objTy =
    defineStructType("Foo", 3, UInt64Type(), UInt64Type(), UInt64Type());

void foo(Object *myObj) {
  assertType(&objTy, myObj);

  Field *field1 = getStructField(&objTy, myObj, 0);
  Field *field2 = getStructField(&objTy, myObj, 1);
  Field *field3 = getStructField(&objTy, myObj, 2);

  uint64_t read1 = readUInt64(field1);
  uint64_t read2 = readUInt64(field2);
  uint64_t read3 = readUInt64(field3);
}

int main() {

  Object *myObj = allocateStruct(&objTy);

  //  std::cerr << myObj->toString() << "\n";

  Field *field1 = getStructField(&objTy, myObj, 0);
  Field *field2 = getStructField(&objTy, myObj, 1);
  Field *field3 = getStructField(&objTy, myObj, 2);

  writeUInt64(field1, 123);
  writeUInt64(field2, 456);
  writeUInt64(field3, 789);

  foo(myObj);
}
