#include <iostream>

#include "memoir.h"

using namespace memoir;

auto strctA = StructType("A");
auto strctB = StructType("B");

Type *aTy = defineStructType("A", 2, UInt64Type(), ReferenceType(&strctB));

Type *bTy =
    defineStructType("B", 2, ReferenceType(&strctA), ReferenceType(&strctB));

int main() {
  Object *myA = allocateStruct(&aTy);
  Object *myA2 = allocateStruct(&aTy);
  Object *myB = allocateStruct(&bTy);
  Object *myB2 = allocateStruct(&bTy);

  Field *aField1 = getStructField(&aTy, myA, 0);
  Field *aField2 = getStructField(&aTy, myA, 1);

  // a->i = 0;
  writeUInt64(aField1, 0);
  // a->b = &b;
  writeReference(aField2, myB);

  Field *bField1 = getStructField(&bTy, myB, 0);
  Field *bField2 = getStructField(&bTy, myB, 1);

  // b->a = &a;
  writeReference(bField1, myA);
  // b->b = &b2;
  writeReference(bField2, myB2);

  Field *a2Field1 = getStructField(&aTy, myA2, 0);
  Field *a2Field2 = getStructField(&aTy, myA2, 1);

  // a2->i = 1;
  writeUInt64(a2Field1, 1);
  // a2->b = &b2;
  writeReference(a2Field2, myB2);

  Field *b2Field1 = getStructField(&bTy, myB2, 0);
  Field *b2Field2 = getStructField(&bTy, myB2, 1);

  // b2->a = &a2;
  writeReference(b2Field1, myA2);
  // b2->b = &b;
  writeReference(b2Field2, myB);

  // print(b2->b->a->i)
  // b2->b
  Object *ptrToB = readReference(b2Field2);

  // b2->b->a
  Field *tmpField = getStructField(&bTy, ptrToB, 0);
  Object *ptrToA = readReference(tmpField);

  // b2->b->a->i
  tmpField = getStructField(&aTy, ptrToA, 0);
  uint64_t value = readUInt64(tmpField);

  // Expect 0 to be printed
  std::cout << "Expected value: 0\n";
  std::cout << "  Actual value: " << value << "\n";
}
