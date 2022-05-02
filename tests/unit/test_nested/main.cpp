#include <iostream>

#include "object_ir.h"

using namespace objectir;

Type *aTy =
    nameObjectType("A",
                   2,
                   getUInt64Type(),
                   getPointerType(getNamedType("B")));

Type *bTy =
    nameObjectType("B",
                   1,
                   getUInt64Type());

int main() {
  Object *myA = buildObject(aTy);
  Object *myB = buildObject(bTy);
  Field *aField1 = getObjectField(myA, 0);
  Field *aField2 = getObjectField(myA, 1);

  writeUInt64(aField1, 0);    // a->i = 0;
  writePointer(aField2, myB); // a->b = &b;

  Field *bField1 = getObjectField(myB, 0);
  writeUInt64(bField1, 123);
  Object *myBcopy = readObject(aField2);
  Field *bField1copy = getObjectField(myBcopy, 0);
  std::cerr << readUInt64(bField1copy);
}
