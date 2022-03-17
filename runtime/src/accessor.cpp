/*
 * Object representation recognizable by LLVM IR
 * This file contains methods to access object-ir
 * objects, fields and types for the object-ir library.
 *
 * Author(s): Tommy McMichen
 * Created: Mar 17, 2022
 */

#include "accessor.hpp"

using namespace objectir;

extern "C" {

/*
 * Object accesses
 */
Field *getObjectField(Object *object, uint64_t fieldNo) {
  // TODO
  return nullptr;
}

/*
 * Array accesses
 */
Field *getArrayElement(Array *array, uint64_t index) {
  // TODO
  return nullptr;
}

/*
 * Union accesses
 */
Field *getUnionMember(Union *unionObj, uint64_t index) {
  // TODO
  return nullptr;
}

/*
 * Field accesses
 */
// Unsigned integer access
void writeUInt64(IntegerField *field, uint64_t value) {
  // TODO
}
void writeUInt32(IntegerField *field, uint32_t value) {
  // TODO
}
void writeUInt16(IntegerField *field, uint16_t value) {
  // TODO
}
void writeUInt8(IntegerField *field, uint8_t value) {
  // TODO
}

// Signed integer access
void writeInt64(IntegerField *field, int64_t value) {
  // TODO
}
void writeInt32(IntegerField *field, int32_t value) {
  // TODO
}
void writeInt16(IntegerField *field, int16_t value) {
  // TODO
}
void writeInt8(IntegerField *field, int8_t value) {
  // TODO
}

// Floating point access
void writeFloat(FloatField *field, float value) {
  // TODO
}
void writeDouble(DoubleField *field, double value) {
  // TODO
}

// Pointer access
__attribute__((noinline)) void writeObject(
    ObjectField *field,
    Object *object) {
  // TODO
}

// Unsigned integer access
uint64_t getUInt64(IntegerField *field) {
  // TODO
  return (uint64_t)field->value;
}
uint32_t getUInt32(IntegerField *field) {
  // TODO
  return (uint32_t)field->value;
}
uint16_t getUInt16(IntegerField *field) {
  // TODO
  return (uint16_t)field->value;
}
uint8_t getUInt8(IntegerField *field) {
  // TODO
  return (uint64_t)field->value;
}

// Signed integer access
int64_t getInt64(IntegerField *field) {
  // TODO
  return (int64_t)field->value;
}
int32_t getInt32(IntegerField *field) {
  // TODO
  return (int32_t)field->value;
}
int16_t getInt16(IntegerField *field) {
  // TODO
  return (int16_t)field->value;
}
int8_t getInt8(IntegerField *field) {
  // TODO
  return (int8_t)field->value;
}

// Floating point access
float getFloat(FloatField *field) {
  // TODO
  return field->value;
}
double getDouble(DoubleField *field) {
  // TODO
  return field->value;
}

// Pointer access
Object *getObject(ObjectField *field) {
  // TODO
  return nullptr;
}

} // extern "C"
