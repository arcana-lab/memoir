#include <iostream>

#include "object_ir.h"

using namespace objectir;

#define ARR_LEN 1000

int main(int argc, char **argv) {

  uint64_t iterations;
  if (argc < 2) {
    iterations = ARR_LEN;
  } else {
    iterations = (uint64_t)atoi(argv[1]);
  }

  Type *objTy = getObjectType(3,
                              getUInt64Type(),
                              getUInt64Type(),
                              getUInt64Type());
  Type *arrayTy = getArrayType(objTy);

  Array *myArr = buildArray(arrayTy, iterations);

  // Initialize each object
  for (int i = 0; i < iterations; i++) {
    Object *elem = buildObject(objTy);

    writeUInt64(getObjectField(elem, 0), rand());
    writeUInt64(getObjectField(elem, 1), rand());
    writeUInt64(getObjectField(elem, 2), rand());

    writeObject(getArrayElement(myArr, i), elem);
  }

  /*
  for (int i = 0; i < iterations; i++) {
  #if OPTIMIZED
    ptr->a[i % iterations] += ptr->a[(i - 1) % iterations];
  #else
    ptr[i].a += ptr[(i - 1) % iterations].a;
  #endif
  }
  */

  // Perform computation for field 1
  for (int i = 0; i < iterations; i++) {
    auto obj1 = readObject(getArrayElement(myArr, i));
    auto obj2 = readObject(
        getArrayElement(myArr, (i - 1) % iterations));

    auto field1 = getObjectField(obj1, 0);
    auto field2 = getObjectField(obj2, 0);

    uint64_t a1 = readUInt64(field1);
    uint64_t a2 = readUInt64(field2);

    writeUInt64(field1, a1 + a2);
  }

  /*
  // MAP b
  for (int i = 0; i < iterations; i++) {
#if OPTIMIZED
    ptr->b[i % iterations] += ptr->b[(i + 1) % iterations];
#else
    ptr[i].b += ptr[(i + 1) % iterations].b;
#endif
  }
  */

  for (int i = 0; i < iterations; i++) {
    auto obj1 = readObject(getArrayElement(myArr, i));
    auto obj2 = readObject(
        getArrayElement(myArr, (i + 1) % iterations));

    auto field1 = getObjectField(obj1, 1);
    auto field2 = getObjectField(obj2, 1);

    uint64_t b1 = readUInt64(field1);
    uint64_t b2 = readUInt64(field2);

    writeUInt64(field1, b1 + b2);
  }

  /*
  // MAP c
  for (int i = 0; i < iterations; i++) {
#if OPTIMIZED
    ptr->c[i % iterations] =
        ptr->a[i % iterations] & ptr->b[i % iterations];
#else
    ptr[i % iterations].c =
        ptr[i % iterations].a & ptr[i % iterations].b;
#endif
  }
  */
  for (int i = 0; i < iterations; i++) {
    auto obj = readObject(getArrayElement(myArr, i));

    auto fieldA = getObjectField(obj, 0);
    auto fieldB = getObjectField(obj, 1);
    auto fieldC = getObjectField(obj, 2);

    uint64_t a = readUInt64(fieldA);
    uint64_t b = readUInt64(fieldB);

    writeUInt64(fieldC, a & b);
  }

  /*
  // REDUCE
  int max = 0;
  for (int i = 0; i < iterations; i++) {
    int c;
#if OPTIMIZED
    c = ptr->c[i % iterations];
#else
    c = ptr[i % iterations].c;
#endif

    if (c > max) {
      max = c;
    }
  }
  */
  uint64_t max = 0;
  for (int i = 0; i < iterations; i++) {
    auto obj = readObject(getArrayElement(myArr, i));

    auto fieldC = getObjectField(obj, 2);

    uint64_t c = readUInt64(fieldC);

    if (c > max) {
      max = c;
    }
  }

  std::cout << "Max: " << max << "\n";
}
