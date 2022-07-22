#include <iostream>

#include "memoir.h"

using namespace memoir;

Type *objTy =
    defineStructType("Foo", 3, UInt64Type(), UInt64Type(), UInt64Type());

Object *main_foo(Object *myObj) {
  setReturnType(objTy);
  assertType(objTy, myObj);

  Field *field1 = getStructField(myObj, 0);
  Field *field2 = getStructField(myObj, 1);
  Field *field3 = getStructField(myObj, 2);

  uint64_t read1 = readUInt64(field1);
  uint64_t read2 = readUInt64(field2);
  uint64_t read3 = readUInt64(field3);

  Object *newObj = allocateStruct(objTy);

  Field *newField1 = getStructField(newObj, 0);
  Field *newField2 = getStructField(newObj, 1);
  Field *newField3 = getStructField(newObj, 2);

  writeUInt64(newField1, read1 + read2);
  writeUInt64(newField2, read2 + read3);
  writeUInt64(newField3, read3 + read1);

  return newObj;
}

int main() {

  Object *myObj = allocateStruct(objTy);

  // std::cerr << myObj->toString() << "\n";

  Field *field1 = getStructField(myObj, 0);
  Field *field2 = getStructField(myObj, 1);
  Field *field3 = getStructField(myObj, 2);

  writeUInt64(field1, 123);
  writeUInt64(field2, 456);
  writeUInt64(field3, 789);

  Object *newObj = main_foo(myObj);

  Field *newField1 = getStructField(newObj, 0);
  Field *newField2 = getStructField(newObj, 1);
  Field *newField3 = getStructField(newObj, 2);

  uint64_t old1 = readUInt64(field1);
  uint64_t old2 = readUInt64(field2);
  uint64_t old3 = readUInt64(field3);

  uint64_t new1 = readUInt64(newField1);
  uint64_t new2 = readUInt64(newField2);
  uint64_t new3 = readUInt64(newField3);

  std::cout << "Old Object: \n";
  std::cout << "  1: " << old1 << "\n";
  std::cout << "  2: " << old2 << "\n";
  std::cout << "  3: " << old3 << "\n";

  std::cout << "\n======================\n";
  std::cout << "New Object: \n";
  std::cout << "  1: " << new1 << "\n";
  std::cout << "  2: " << new2 << "\n";
  std::cout << "  3: " << new3 << "\n";
}
