#include <iostream>

#include "object_ir.h"
#include <cstdlib>

using namespace objectir;

Type *objTy = getObjectType(3,
                            getUInt64Type(),
                            getUInt64Type(),
                            getUInt64Type());

void main_2(Object* obj)
{
  assertType(objTy, obj);
  Field* f = getObjectField(obj, 1);
  long x =readUInt64(f);
  long k = 0;
  for(int i =0; i < x; ++i)
  {    
    k++;
  }
}

int main() {

  Object* obj4 = buildObject(objTy);
  Field *field41 = getObjectField(obj4, 0);
  writeUInt64(field41, 10000);
  main_2(obj4);
  // int counter = 0;
  // while(counter<2)
  // {
  //     counter++;

  //     // alocate and free obj4 inside the loop:
  //     Object* obj4 = buildObject(objTy);
  //     Field *field41 = getObjectField(obj4, 0);
  //     writeUInt64(field41, 10000);
  //     std::cerr << "obj4 =: " << readUInt64(field41) << "\n";
  //     Object* innerObj = nullptr;
  //     for(int k = 0; k<10; ++k)
  //     {
  //       innerObj = buildObject(objTy);
  //       Field *fieldinner3 = getObjectField(obj4, 2);
  //       writeUInt64(fieldinner3, k);
  //       std::cerr << "innerObj =: " << readUInt64(fieldinner3) << "\n";
  //     }       
  //     deleteObject(innerObj);
      
  //     deleteObject(obj4);   

  // }
    return 0;

}
