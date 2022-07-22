#include <iostream>

#include "memoir.h"

using namespace memoir;

Type *aTy =
    defineStructType("A", 2, UInt64Type(), ReferenceType(StructType("B")));

Type *bTy = defineStructType("B",
                             2,
                             ReferenceType(StructType("A")),
                             ReferenceType(StructType("B")));

int main() {
  Object *myA = allocateStruct(aTy);
  Object *myA2 = allocateStruct(aTy);
  Object *myB = allocateStruct(bTy);
  Object *myB2 = allocateStruct(bTy);

  Field *aField1 = getStructField(myA, 0);
  Field *aField2 = getStructField(myA, 1);

  // a->i = 0;
  writeUInt64(aField1, 0);
  // a->b = &b;
  writeReference(aField2, myB);

  Field *bField1 = getStructField(myB, 0);
  Field *bField2 = getStructField(myB, 1);

  // b->a = &a;
  writeReference(bField1, myA);
  // b->b = &b2;
  writeReference(bField2, myB2);

  Field *a2Field1 = getStructField(myA2, 0);
  Field *a2Field2 = getStructField(myA2, 1);

  // a2->i = 1;
  writeUInt64(a2Field1, 1);
  // a2->b = &b2;
  writeReference(a2Field2, myB2);

  Field *b2Field1 = getStructField(myB2, 0);
  Field *b2Field2 = getStructField(myB2, 1);

  // b2->a = &a2;
  writeReference(b2Field1, myA2);
  // b2->b = &b;
  writeReference(b2Field2, myB);

  // print(b2->b->a->i)
  // b2->b
  Object *ptrToB = readReference(b2Field2);

  // b2->b->a
  Field *tmpField = getStructField(ptrToB, 0);
  Object *ptrToA = readReference(tmpField);

  // b2->b->a->i
  tmpField = getStructField(ptrToA, 0);
  uint64_t value = readUInt64(tmpField);

  // Expect 0 to be printed
  std::cout << "Expected value: 0\n";
  std::cout << "  Actual value: " << value << "\n";
}
