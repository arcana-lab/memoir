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

  Object *myObj1 = buildObject(objTy);
  Object *myObj2 = buildObject(objTy);
  Field *field10 = getObjectField(myObj1, 0);
  Field *field20 = getObjectField(myObj2, 0);
  writeUInt64(field10, 321);
  writeUInt64(field20, 123);
  int counter = 0;
  while(counter<2)
  {
      counter++;
      Object* object3 = myObj1;
      myObj1 = myObj2;
      myObj2 = object3;
      Field *field1 = getObjectField(myObj1, 0);
      writeUInt64(field1, 456);
  }
    Field *field1 = getObjectField(myObj1, 0);
    int64_t x = readUInt64(field1);
    std::cerr << "x=: " << x << "\n";
    return x;

}
