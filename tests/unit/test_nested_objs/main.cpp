#include <iostream>

#include "object_ir.h"

using namespace objectir;

int main() {
  Type *innerTy = getObjectType(1, getUInt64Type());  
  Type *outerTy = getObjectType(1, innerTy);
    
  Object *innerObj = buildObject(innerTy);
  Field *innerField = getObjectField(innerObj, 0);
  writeUInt64(innerField, 123);

  Object *outerObj = buildObject(outerTy);
  Field *outField = getObjectField(outerObj, 0);
  writeObject(outField, innerObj);  

  return 0;
}
