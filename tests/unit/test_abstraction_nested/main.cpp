#include <iostream>
#include "object_ir.h"

using namespace objectir;

Type *innerType = getObjectType(2,
                                getUInt64Type(),
                                getUInt64Type());

Type *outerType = getObjectType(2,
                                innerType,
                                getUInt64Type());

int main () {
  Object *outerObject = buildObject(outerType);
  Field *field1 = getObjectField(outerObject, 0);
  Object *innerObject = readObject(field1);
  Field *field11 = getObjectField(innerObject, 0);
  writeUInt64(field11, 10);
  Field *field12 = getObjectField(innerObject, 1);
  writeUInt64(field12, 20);
  
  Field *field2 = getObjectField(outerObject, 1);
  writeUInt64(field2, 30);

  std::cout << readUInt64(field11) + readUInt64(field12) + readUInt64(field2) << std::endl;

  return 0;
}
