#include <iostream>

#include "object_ir.h"
#include <cstdlib>

using namespace objectir;

Type *objTy = getObjectType(3,
                            getUInt64Type(),
                            getUInt64Type(),
                            getUInt64Type());

char* _ignore()
{
    return (char*) malloc(1);
}

int main() {

  Object *myObj = buildObject(objTy);

}
