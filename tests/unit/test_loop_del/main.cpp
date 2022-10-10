#include <iostream>

#include "memoir.h"
#include <cstdlib>

using namespace memoir;

Type *objTy =
    defineStructType("Foo", 3, UInt64Type(), UInt64Type(), UInt64Type());

int main() {

  Object *myObj1 = allocateStruct(&objTy);
  Object *myObj2 = allocateStruct(&objTy);
  Field *field10 = getStructField(&objTy, myObj1, 0);
  Field *field11 = getStructField(&objTy, myObj1, 1);
  Field *field12 = getStructField(&objTy, myObj1, 2);
  Field *field20 = getStructField(&objTy, myObj2, 0);
  Field *field21 = getStructField(&objTy, myObj2, 1);
  Field *field22 = getStructField(&objTy, myObj2, 2);
  writeUInt64(field10, 0);
  writeUInt64(field11, 0);
  writeUInt64(field12, 0);
  writeUInt64(field20, 0);
  writeUInt64(field21, 0);
  writeUInt64(field22, 0);
  int counter = 0;
  while (counter < 2) {
    counter++;

    // alocate and free obj4 inside the loop:
    Object *obj4 = allocateStruct(&objTy);
    Field *field41 = getStructField(&objTy, obj4, 0);
    writeUInt64(field41, 10000);
    std::cerr << "obj4 =: " << readUInt64(field41) << "\n";
    deleteObject(obj4);

    Object *object3 = myObj1;

    myObj1 = myObj2;
    myObj2 = object3;
    Field *field1 = getStructField(&objTy, myObj1, 0);
    Field *field2 = getStructField(&objTy, myObj1, 1);
    Field *field3 = getStructField(&objTy, myObj1, 2);
    writeUInt64(field1, 10);
    writeUInt64(field2, 20);
    writeUInt64(field3, 40);
  }
  Field *field1 = getStructField(&objTy, myObj1, 0);
  Field *field2 = getStructField(&objTy, myObj1, 1);
  Field *field3 = getStructField(&objTy, myObj1, 2);
  std::cerr << "1=: " << readUInt64(field1) << "\n";
  std::cerr << "2=: " << readUInt64(field2) << "\n";
  std::cerr << "3=: " << readUInt64(field3) << "\n";
  return 0;
}
