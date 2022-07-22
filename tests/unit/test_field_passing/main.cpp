#include <iostream>

#include "memoir.h"

using namespace memoir;

Type *innerTy = defineStructType("Foo", 1, UInt64Type());

Type *objTy =
    defineStructType("Bar", 3, UInt64Type(), UInt64Type(), UInt64Type());

uint64_t foo(Field *fld) {
  assertType(UInt64Type(), fld);
  writeUInt64(fld, readUInt64(fld) + 1);
  return readUInt64(fld);
}

int main() {
  Object *myObj = allocateStruct(objTy);

  Field *field1 = getStructField(myObj, 0);
  Field *field2 = getStructField(myObj, 1);
  Field *field3 = getStructField(myObj, 2);

  writeUInt64(field1, 123);
  writeUInt64(field2, 456);
  writeUInt64(field3, 789);

  uint64_t read1 = readUInt64(field1);
  uint64_t read2 = readUInt64(field2);
  uint64_t read3 = readUInt64(field3);

  std::cerr << "1: " << read1 << "\n";
  std::cerr << "2: " << read2 << "\n";
  std::cerr << "3: " << read3 << "\n\n";

  writeUInt64(field1, read1 + read2);
  writeUInt64(field2, read2 + read3);
  foo(field3);

  read1 = readUInt64(field1);
  read2 = readUInt64(field2);
  read3 = readUInt64(field3);

  std::cerr << "1: " << read1 << "\n";
  std::cerr << "2: " << read2 << "\n";
  std::cerr << "3: " << read3 << "\n\n";
}
