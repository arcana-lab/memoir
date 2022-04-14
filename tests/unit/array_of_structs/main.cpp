#include <iostream>

#include "object_ir.h"

using namespace objectir;

#define ARR_LEN 1000

int main() {
  Type *objTy = getObjectType(3, getUInt64Type(), getUInt64Type(), getUInt64Type());
  Type *arrayTy = getArrayType(objType);

  Array *myArr = buildArray(myObj, ARR_LEN);

  // Initialize each object
  for (int i = 0; i < ARR_LEN; i++) {
    Object *myObj = buildObject(objTy);

    writeUInt64(getObjectField(elem, 0), rand());
    writeUInt64(getObjectField(elem, 1), rand());
    writeUInt64(getObjectField(elem, 2), rand());
  }

  // Perform computation for field 1
  uint64_t tmp = 0;
  for (int i = 0; i < ARR_LEN; i++) {
    Field *elem = readObject(getArrayElement(myArr, i));
  }

  uint64_t read1 = readUInt64(field1);
  uint64_t read2 = readUInt64(field2);
  uint64_t read3 = readUInt64(field3);

  std::cerr << "1: " << read1 << "\n";
  std::cerr << "2: " << read2 << "\n";
  std::cerr << "3: " << read3 << "\n\n";

  writeUInt64(field1, read1 + read2);
  writeUInt64(field2, read2 + read3);
  writeUInt64(field3, read3 + read1);
  
  read1 = readUInt64(field1);
  read2 = readUInt64(field2);
  read3 = readUInt64(field3);

  std::cerr << "1: " << read1 << "\n";
  std::cerr << "2: " << read2 << "\n";
  std::cerr << "3: " << read3 << "\n\n";
}
