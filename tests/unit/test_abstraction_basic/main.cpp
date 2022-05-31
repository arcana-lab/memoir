#include <iostream>
#include "object_ir.h"

using namespace objectir;

Type *type = getObjectType(2,
                           getUInt64Type(),
                           getUInt64Type());

int main () {
  Object *object = buildObject(type);
  Field *field1 = getObjectField(object, 0);
  Field *field2 = getObjectField(object, 1);
  writeUInt64(field1, rand());
  writeUInt64(field2, rand());

  std::cout << readUInt64(field1) + readUInt64(field2) << std::endl;

  deleteObject(object);

  return 0;
}
