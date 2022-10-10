#include "memoir.h"
#include <iostream>

using namespace memoir;

Type *type = defineStructType("Foo", 2, UInt64Type(), UInt64Type());

int main() {
  Object *object = allocateStruct(&type);
  Field *field1 = getStructField(&type, object, 0);
  Field *field2 = getStructField(&type, object, 1);
  writeUInt64(field1, rand());
  writeUInt64(field2, rand());

  std::cout << readUInt64(field1) + readUInt64(field2) << std::endl;

  deleteObject(object);

  Object *array = allocateTensor(&type, 1, 10);
  for (int i = 0; i < 10; i++) {
    Field *arrayField = getTensorElement(array, i);
    Object *element = readStruct(arrayField);
    writeUInt64(getStructField(&type, element, 0), rand());
    writeUInt64(getStructField(&type, element, 1), rand());
  }

  deleteObject(array);

  return 0;
}
