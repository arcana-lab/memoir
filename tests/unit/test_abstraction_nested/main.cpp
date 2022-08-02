#include "memoir.h"
#include <iostream>

using namespace memoir;

Type *innerType = defineStructType("Foo", 2, UInt64Type(), UInt64Type());

Type *outerType = defineStructType("Bar", 2, UInt64Type(),innerType,UInt64Type());

int main() {
  Object *outerObject = allocateStruct(outerType);
  Field *field1 = getStructField(outerObject, 1);
  Object *innerObject = readStruct(field1);
  Field *field11 = getStructField(innerObject, 1);
  writeUInt64(field11, 10);
  Field *field12 = getStructField(innerObject, 0);
  writeUInt64(field12, 20);

  Field *field2 = getStructField(outerObject, 0);
  writeUInt64(field2, 30);

  std::cout << readUInt64(field11) + readUInt64(field12) + readUInt64(field2)
            << std::endl;

  return 0;
}
