#include <iostream>

#include "memoir.h"
#include <cstdlib>

using namespace memoir;

Type *objTy =
    defineStructType("Foo", 3, UInt64Type(), UInt64Type(), UInt64Type());

long main_2(Object *obj) {
  assertType(objTy, obj);
  long k = 0;
  for (int i = 0; i < 10; i++) {
    Object *obj = allocateStruct(objTy);
    Field *f = getStructField(obj, 0);
    writeUInt64(f, 10000);
    k += readUInt64(f);
  }
  return k;
}

int main() {

  Object *obj = allocateStruct(objTy);

  std::cerr << main_2(obj);
  // int counter = 0;
  // while(counter<2)
  // {
  //     counter++;

  //     // alocate and free obj4 inside the loop:
  //     Object* obj4 = allocateStruct(objTy);
  //     Field *field41 = getStructField(obj4, 0);
  //     writeUInt64(field41, 10000);
  //     std::cerr << "obj4 =: " << readUInt64(field41) << "\n";
  //     Object* innerObj = nullptr;
  //     for(int k = 0; k<10; ++k)
  //     {
  //       innerObj = allocateStruct(objTy);
  //       Field *fieldinner3 = getStructField(obj4, 2);
  //       writeUInt64(fieldinner3, k);
  //       std::cerr << "innerObj =: " << readUInt64(fieldinner3) << "\n";
  //     }
  //     deleteObject(innerObj);

  //     deleteObject(obj4);

  // }
  return 0;
}
