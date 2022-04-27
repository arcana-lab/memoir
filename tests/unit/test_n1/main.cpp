#include <iostream>

#include "object_ir.h"

using namespace objectir;

Type *objTy = getObjectType(3,
                            getUInt64Type(),
                            getUInt64Type(),
                            getUInt64Type());

int main() {


  Object *myObj = buildObject(objTy);


}
