#include <iostream>

#include "object_ir.h"

using namespace objectir;

Type *objTy = getObjectType(1, getUInt64Type());

int main() {
  Object *the_phi;
  Object *b1 = buildObject(objTy);
  Object *b2 = buildObject(objTy);

  writeUInt64(getObjectField(b1, 0), 42);
  writeUInt64(getObjectField(b2, 0), 73);

  if (true) {
    the_phi = b1;
  } else {
    the_phi = b2;
  }

  uint64_t read1 = readUInt64(getObjectField(the_phi, 0));

  return 0;
}
