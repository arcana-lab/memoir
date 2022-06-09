#include <iostream>

#include "object_ir.h"
#include <cstdlib>

using namespace objectir;

Type *objTy = getObjectType(3,
                            getUInt64Type(),
                            getUInt64Type(),
                            getUInt64Type());

int main() {

  Object *myObj1 = buildObject(objTy);
  Object *myObj2 = buildObject(objTy);
  Field *field10 = getObjectField(myObj1, 0);
  Field *field11 = getObjectField(myObj1, 1);
  Field *field12 = getObjectField(myObj1, 2);
  Field *field20 = getObjectField(myObj2, 0);
  Field *field21 = getObjectField(myObj2, 1);
  Field *field22 = getObjectField(myObj2, 2);
  writeUInt64(field10, 0);
  writeUInt64(field11, 0);
  writeUInt64(field12, 0);
  writeUInt64(field20, 0);
  writeUInt64(field21, 0);
  writeUInt64(field22, 0);
  int counter = 0;
  while(counter<2)
  {
      counter++;

      // alocate and free obj4 inside the loop:
      Object* obj4 = buildObject(objTy);
      Field *field41 = getObjectField(obj4, 0);
      writeUInt64(field41, 10000);
      std::cerr << "obj4 =: " << readUInt64(field41) << "\n";
      deleteObject(obj4);     

      Object* object3 = myObj1;
  
      myObj1 = myObj2;
      myObj2 = object3;
      Field *field1 = getObjectField(myObj1, 0);
      Field *field2 = getObjectField(myObj1, 1);
      Field *field3 = getObjectField(myObj1, 2);
      writeUInt64(field1, 10);
      writeUInt64(field2, 20);
      writeUInt64(field3, 40);
  }
    Field *field1 = getObjectField(myObj1, 0);
    Field *field2 = getObjectField(myObj1, 1);
    Field *field3 = getObjectField(myObj1, 2);
    std::cerr << "1=: " << readUInt64(field1) << "\n";
    std::cerr << "2=: " << readUInt64(field2) << "\n";
    std::cerr << "3=: " << readUInt64(field3) << "\n";
    return 0;

}