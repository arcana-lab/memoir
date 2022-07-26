#include "memoir.h"
#include <iostream>

using namespace memoir;

Type *type = defineStructType("Foo", 2, UInt64Type(), UInt64Type());

int main() {
  Object *object = allocateStruct(type);
  Field *field1 = getStructField(object, 0);
  Field *field2 = getStructField(object, 1);
  writeUInt64(field1, rand());
  writeUInt64(field2, rand());

  std::cout << readUInt64(field1) + readUInt64(field2) << std::endl;

  deleteObject(object);

  return 0;
}
