

#include <iostream>

#include "object_ir.h"

using namespace objectir;

Type *objTy = getObjectType(1, getUInt64Type());

int main() {
  Object *b1 = buildObject(objTy);
  Object *b2 = buildObject(objTy);

  Field* f1 = getObjectField(b1, 0);
  Field* f2 = getObjectField(b1, 0);
  
  writeUInt64(f1, 42);
  writeUInt64(f2, 73);

  if (readUInt64(f1) == readUInt64(f2)) {
    b2 = b1;
  } else {
    std::cout << "an important thing\n";
  }

  uint64_t read1 = readUInt64(getObjectField(b2, 0));

  return 0;
}
