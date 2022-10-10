#include <iostream>
#include <string>

#include "memoir.h"

using namespace memoir;

Type *strct = StructType("B");
Type *aTy = defineStructType("A", 2, UInt64Type(), ReferenceType(&strct));

Type *bTy = defineStructType("B", 1, UInt64Type());

int main() {
  Object *myA = allocateStruct(&aTy);
  Object *myB = allocateStruct(&bTy);
  Field *aField1 = getStructField(&aTy, myA, 0);
  Field *aField2 = getStructField(&aTy, myA, 1);

  writeUInt64(aField1, 0);      // a->i = 0;
  writeReference(aField2, myB); // a->b = &b;

  Field *bField1 = getStructField(&bTy, myB, 0);
  writeUInt64(bField1, 123);
  Object *myBcopy = readReference(aField2);
  Field *bField1copy = getStructField(&bTy, myBcopy, 0);
  std::cerr << std::to_string(readUInt64(bField1copy));
}
