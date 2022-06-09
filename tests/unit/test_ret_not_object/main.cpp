#include <iostream>

#include "object_ir.h"

using namespace objectir;

Type *objTy = getObjectType(3,
                            getUInt64Type(),
                            getUInt64Type(),
                            getUInt64Type());

 void foo(Object *myObj) {
  assertType(objTy, myObj);

  Field *field1 = getObjectField(myObj, 0);
  Field *field2 = getObjectField(myObj, 1);
  Field *field3 = getObjectField(myObj, 2);

  uint64_t read1 = readUInt64(field1);
  uint64_t read2 = readUInt64(field2);
  uint64_t read3 = readUInt64(field3);

}

int main() {

  Object *myObj = buildObject(objTy);

//  std::cerr << myObj->toString() << "\n";

  Field *field1 = getObjectField(myObj, 0);
  Field *field2 = getObjectField(myObj, 1);
  Field *field3 = getObjectField(myObj, 2);

  writeUInt64(field1, 123);
  writeUInt64(field2, 456);
  writeUInt64(field3, 789);

  foo(myObj);

}
