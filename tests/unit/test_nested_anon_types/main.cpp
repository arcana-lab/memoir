#include <iostream>

#include "object_ir.h"

using namespace objectir;

Type *innerTy =
    getObjectType(2, getUInt64Type(), getUInt64Type());

Type *objTy = getObjectType(2,
                            getUInt64Type(),
                            getPointerType(innerTy));

int main() {

  Object *innerObj = buildObject(innerTy);

  Object *myObj = buildObject(objTy);

  Field *field1 = getObjectField(myObj, 0);
  Field *field2 = getObjectField(myObj, 1);

  // Update the outer fields
  writeUInt64(field1, 123);
  writePointer(field2, innerObj);

  // Get the object from the pointer
  Object *deref = readPointer(field2);

  // Fetch the fields of the inner object
  Field *inner1 = getObjectField(deref, 0);
  Field *inner2 = getObjectField(deref, 1);

  // Update the inner fields
  writeUInt64(inner1, 456);
  writeUInt64(inner2, 789);

  uint64_t read1 = readUInt64(field1);
  uint64_t read2 = readUInt64(inner1);
  uint64_t read3 = readUInt64(inner2);

  writeUInt64(field1, read1 + read2);
  writeUInt64(inner1, read2 + read3);
  writeUInt64(inner2, read3 + read1);

  read1 = readUInt64(field1);
  read2 = readUInt64(inner1);
  read3 = readUInt64(inner2);

  std::cout << "read1: " << read1 << "\n";
  std::cout << "read2: " << read2 << "\n";
  std::cout << "read3: " << read3 << "\n";
}
