#include <iostream>
#include "object_ir.h"

using namespace objectir;

#define ARR_LEN 1000

// typedef struct {
//   uint64_t value3;
// } innerObject_t;
Type *innerObject_t = getObjectType(1,
                                    getUInt64Type());

// typedef struct {
//   uint64_t value1;
//   uint64_t value2;
//   innerObject_t *innerObject;
// } outerObject_t;
Type *outerObject_t = getObjectType(3,
                                    getUInt64Type(),
                                    getUInt64Type(),
                                    innerObject_t);

int main (int argc, char **argv) {
  uint64_t size;
  if (argc < 2) {
    size = ARR_LEN;
  } else {
    size = (uint64_t)atoi(argv[1]);
  }

std::cout << "CP0\n";

  // outerObject_t *outerObjects = (outerObject_t *)malloc(size * sizeof(outerObject_t));
  Type *outerArray_t = getArrayType(outerObject_t);
  Array *outerObjects = buildArray(outerArray_t, size);
  for (uint64_t i = 0; i < size; i++) {
    Object *outerObject = buildObject(outerObject_t);
    Field *outerArrayField = getArrayElement(outerObjects, i);
    writeObject(outerArrayField, outerObject);
  }

  // innerObject_t *innerObjects = (innerObject_t *)malloc(size * sizeof(innerObject_t));
  Type *innerArray_t = getArrayType(innerObject_t);
  Array *innerObjects = buildArray(innerArray_t, size);
  for (uint64_t i = 0; i < size; i++) {
    Object *innerObject = buildObject(innerObject_t);
    Field *innerArrayField = getArrayElement(innerObjects, i);
    writeObject(innerArrayField, innerObject);
  }

std::cout << "CP1\n";

  // Assign inner objects to outer ones
  for (uint64_t i = 0; i < size; i++) {
    // outerObjects[i].innerObject = &innerObjects[i];
    Object *outerObject = readObject(getArrayElement(outerObjects, i));
    Field *outerObjectField3 = getObjectField(outerObject, 2);
    Object *innerObject = readObject(getArrayElement(innerObjects, i));
    writeObject(outerObjectField3, innerObject);
  }

std::cout << "CP2\n";

  // Computation part, value3 = value1 + value2
  for (uint64_t i = 0; i < size; i++) {
    // outerObjects[i].value1 = rand();
    // outerObjects[i].value2 = rand();
    // outerObjects[i].innerObject->value3 = outerObjects[i].value1 + outerObjects[i].value2;
    Object *outerObject = readObject(getArrayElement(outerObjects, i));
    Field *outerObjectField1 = getObjectField(outerObject, 0);
    Field *outerObjectField2 = getObjectField(outerObject, 1);
    writeUInt64(outerObjectField1, rand());
    writeUInt64(outerObjectField2, rand());

    Object *innerObject = readObject(getObjectField(outerObject, 2));
    Field *innerObjectField = getObjectField(innerObject, 0);
    writeUInt64(innerObjectField, readUInt64(outerObjectField1) + readUInt64(outerObjectField2));
  }

std::cout << "CP3\n";

  // Find maximum value3 and index
  uint64_t max = 0;
  uint64_t max_index = 0;
  for (uint64_t i = 1; i < size; i++) {
    // if (outerObjects[i].innerObject->value3 > max) {
    //   max = outerObjects[i].innerObject->value3;
    //   max_index = i;
    // }
    Object *outerObject = readObject(getArrayElement(outerObjects, i));
    Object *innerObject = readObject(getObjectField(outerObject, 2));
    Field *innerField = getObjectField(innerObject, 0);
    if (readUInt64(innerField) > max) {
      max = readUInt64(innerField);
      max_index = i;
    }
  }

std::cout << "CP4\n";

  std::cout << "Maximun: " << max << ", Index: " << max_index << std::endl;
}