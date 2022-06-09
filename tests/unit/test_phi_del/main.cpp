#include <iostream>

#include "object_ir.h"

using namespace objectir;

Type *objTy = getObjectType(1,
                            getUInt64Type());

int main() {
  Object *myObj1 = buildObject(objTy);
 Object *myObj2 = buildObject(objTy);
 Object *myObj3; // = buildObject(objTy);


  Field *field1 = getObjectField(myObj1, 0);
  Field *field2 = getObjectField(myObj2, 0);

  writeUInt64(field1, 123);
  writeUInt64(field2, 456);

  if (readUInt64(field1) == 0) {
    myObj3 = myObj2;
  } else {
    myObj3 = myObj1;
  }
  deleteObject(myObj3);

}