/*
 * Object representation recognizable by LLVM IR
 * This file contains methods to access object-ir
 * objects, fields and types for the object-ir library.
 *
 * Author(s): Tommy McMichen
 * Created: Mar 17, 2022
 */

#include "accessor.h"

using namespace objectir;

extern "C" {

/*
 * Object accesses
 */
Field *getObjectField(Object *object, uint64_t fieldNo) {
  return object->fields.at(fieldNo);
}

/*
 * Array accesses
 */
Field *getArrayElement(Array *array, uint64_t index) {
  return array->fields.at(index);
}

/*
 * Union accesses
 */
Field *getUnionMember(Union *unionObj, uint64_t index) {
  return unionObj->members.at(index);
}

} // extern "C"

/*
 * Field accesses
 */
// Unsigned integer access
void write(IntegerField *field, uint64_t value) {
  field->value = value;
}
void write(IntegerField *field, uint32_t value) {
  field->value = (uint64_t)value;
}
void write(IntegerField *field, uint16_t value) {
  field->value = (uint64_t)value;
}
void write(IntegerField *field, uint8_t value) {
  field->value = (uint64_t)value;
}

// Signed integer access
void write(IntegerField *field, int64_t value) {
  field->value = (uint64_t)value;
}
void write(IntegerField *field, int32_t value) {
  field->value = (uint64_t)value;
}
void write(IntegerField *field, int16_t value) {
  field->value = (uint64_t)value;
}
void write(IntegerField *field, int8_t value) {
  field->value = (uint64_t)value;
}

// Floating point access
void write(FloatField *field, float value) {
  field->value = value;
}
void write(DoubleField *field, double value) {
  field->value = value;
}

// Pointer access
__attribute__((noinline)) void writeObject(
    ObjectField *field,
    Object *object) {
  field->value = object;
}

// Integer access
uint64_t read(IntegerField *field) {
  return field->value;
}

// Floating point access
float read(FloatField *field) {
  return field->value;
}
double read(DoubleField *field) {
  return field->value;
}

// Pointer access
Object *read(ObjectField *field) {
  return field->value;
}
