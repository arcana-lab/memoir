#include <iostream>
#include "object_ir.h"

using namespace objectir;

Type *type = getObjectType(2,
                           getUInt64Type(),
                           getUInt64Type());
Type *arrayType = getArrayType(type);

int main () {
  Object *object = buildObject(type);
  Field *field1 = getObjectField(object, 0);
  Field *field2 = getObjectField(object, 1);
  writeUInt64(field1, rand());
  writeUInt64(field2, rand());

  std::cout << readUInt64(field1) + readUInt64(field2) << std::endl;

  deleteObject(object);

  Array *array = buildArray(arrayType, 10);
  for (int i = 0; i < 10; i++) {
    Object *obj = buildObject(type);

    writeUInt64(getObjectField(obj, 0), rand());
    writeUInt64(getObjectField(obj, 1), rand());
    Field *arrayField = getArrayElement(array, i);
    writeObject(arrayField, obj);

    deleteObject(obj);
  }

  deleteObject(array);

  return 0;
}
