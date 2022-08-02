#include "memoir.h"
#include <iostream>

using namespace memoir;

Type *innerType = defineStructType("Foo", 2, UInt64Type(), UInt64Type());

Type *outerType = defineStructType("Bar", 2, UInt64Type(),innerType,UInt64Type(),UInt64Type());

int main() {
  Object *outerObject = allocateStruct(outerType);
  return 0;
}
