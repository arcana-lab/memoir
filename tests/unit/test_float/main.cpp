#include <iostream>

#include "memoir.h"

using namespace memoir;

Type *objTy =
    defineStructType("Foo", 3, UInt64Type(), FloatType(), UInt32Type());

int main() {
  Object *myObj = allocateStruct(objTy);

  Field *field1 = getStructField(myObj, 0);
  Field *field2 = getStructField(myObj, 1);
  Field *field3 = getStructField(myObj, 2);

  writeUInt64(field1, 123);
  writeFloat(field2, 45.6);
  writeUInt32(field3, 789);

  uint64_t read1 = readUInt64(field1);
  float read2 = readFloat(field2);
  uint32_t read3 = readUInt32(field3);

  std::cerr << "1: " << read1 << "\n";
  std::cerr << "2: " << read2 << "\n";
  std::cerr << "3: " << read3 << "\n\n";

  writeFloat(field2, read1 + read2);
  writeUInt64(field1, read1 + read3);
  writeUInt32(field3, (uint32_t)read3 + read1);

  read1 = readUInt64(field1);
  read2 = readFloat(field2);
  read3 = readUInt32(field3);

  std::cerr << "1: " << read1 << "\n";
  std::cerr << "2: " << read2 << "\n";
  std::cerr << "3: " << read3 << "\n\n";
}
