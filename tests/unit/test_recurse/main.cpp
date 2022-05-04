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
                   2,
                   getPointerType(getNamedType("A")),
                   getPointerType(getNamedType("B")));

int main() {
  Object *myA = buildObject(aTy);
  Object *myA2 = buildObject(aTy);
  Object *myB = buildObject(bTy);
  Object *myB2 = buildObject(bTy);

  Field *aField1 = getObjectField(myA, 0);
  Field *aField2 = getObjectField(myA, 1);

  // a->i = 0;
  writeUInt64(aField1, 0);
  // a->b = &b;
  writePointer(aField2, myB);

  Field *bField1 = getObjectField(myB, 0);
  Field *bField2 = getObjectField(myB, 1);

  // b->a = &a;
  writePointer(bField1, myA);
  // b->b = &b2;
  writePointer(bField2, myB2);

  Field *a2Field1 = getObjectField(myA2, 0);
  Field *a2Field2 = getObjectField(myA2, 1);

  // a2->i = 1;
  writeUInt64(a2Field1, 1);
  // a2->b = &b2;
  writePointer(a2Field2, myB2);

  Field *b2Field1 = getObjectField(myB2, 0);
  Field *b2Field2 = getObjectField(myB2, 1);

  // b2->a = &a2;
  writePointer(b2Field1, myA2);
  // b2->b = &b;
  writePointer(b2Field2, myB);

  // print(b2->b->a->i)
  // b2->b
  Object *ptrToB = readPointer(b2Field2);

  // b2->b->a
  Field *tmpField = getObjectField(ptrToB, 0);
  Object *ptrToA = readPointer(tmpField);

  // b2->b->a->i
  tmpField = getObjectField(ptrToA, 0);
  uint64_t value = readUInt64(tmpField);

  // Expect 0 to be printed
  std::cout << "Expected value: 0\n";
  std::cout << "  Actual value: " << value << "\n";
}
