#include <iostream>

#include "object_ir.h"
#include <cstdlib>

using namespace objectir;

Type *objTy = getObjectType(3,
                            getUInt64Type(),
                            getUInt64Type(),
                            getUInt64Type());

int main() {

  
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

  }
    return 0;

}
