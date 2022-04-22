#include <iostream>

#include "object_ir.h"

using namespace objectir;

Type *objTy = getObjectType(1, getUInt64Type());

int main() {
  Object *the_phi;
  Object *b1 = buildObject(objTy);
  Object *b2 = buildObject(objTy);

  Field* f1 = getObjectField(b1, 0);
  Field* f2 = getObjectField(b1, 0);
  
  writeUInt64(f1, 42);
  writeUInt64(f2, 73);

  if (readUInt64(f1) == readUInt64(f2)) {
    the_phi = b1;
  } else {
    the_phi = b2;
  }

  uint64_t read1 = readUInt64(getObjectField(the_phi, 0));

  return 0;
}
