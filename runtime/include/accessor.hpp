#pragma once

/*
 * Object representation recognizable by LLVM IR
 * This file contains methods to access object-ir
 * objects, fields and types for the object-ir library.
 *
 * Author(s): Tommy McMichen
 * Created: Mar 11, 2022
 */

#include "objects.hpp"
#include "types.hpp"

namespace objectir {

/*
 * Object accesses
 */
class ObjectAccessor {
  //  extern "C" {
  __attribute__((noinline)) Field *getObjectField(
      Object *object,
      uint64_t fieldNo);
  //  }
};

/*
 * Array accesses
 */
class ArrayAccessor {
  //  extern "C" {
  __attribute__((noinline)) Field *getArrayElement(
      Array *array,
      uint64_t index);
  //}
};

/*
 * Union accesses
 */
class UnionAccessor {
  //  extern "C" {
  __attribute__((noinline)) Field *getUnionMember(
      Union *unionObj,
      uint64_t index);
  //  }
};

/*
 * Field accesses
 */
class FieldAccessor {
  //  extern "C" {
  // Unsigned integer access
  __attribute__((noinline)) void writeUInt64(
      IntegerField *field,
      uint64_t value);
  __attribute__((noinline)) void writeUInt32(
      IntegerField *field,
      uint32_t value);
  __attribute__((noinline)) void writeUInt16(
      IntegerField *field,
      uint16_t value);
  __attribute__((noinline)) void writeUInt8(
      IntegerField *field,
      uint8_t value);

  // Signed integer access
  __attribute__((noinline)) void writeInt64(
      IntegerField *field,
      int64_t value);
  __attribute__((noinline)) void writeInt32(
      IntegerField *field,
      int32_t value);
  __attribute__((noinline)) void writeInt16(
      IntegerField *field,
      int16_t value);
  __attribute__((noinline)) void writeInt8(
      IntegerField *field,
      int8_t value);

  // Floating point access
  __attribute__((noinline)) void writeFloat(
      FloatField *field,
      float value);
  __attribute__((noinline)) void writeDouble(
      DoubleField *field,
      double value);

  // Pointer access
  __attribute__((noinline)) void writeObject(
      ObjectField *field,
      Object *object);

  // Unsigned integer access
  __attribute__((noinline)) uint64_t getUInt64(
      IntegerField *field);
  __attribute__((noinline)) uint32_t getUInt32(
      IntegerField *field);
  __attribute__((noinline)) uint16_t getUInt16(
      IntegerField *field);
  __attribute__((noinline)) uint8_t getUInt8(
      IntegerField *field);

  // Signed integer access
  __attribute__((noinline)) int64_t getInt64(
      IntegerField *field);
  __attribute__((noinline)) int32_t getInt32(
      IntegerField *field);
  __attribute__((noinline)) int16_t getInt16(
      IntegerField *field);
  __attribute__((noinline)) int8_t getInt8(
      IntegerField *field);

  // Floating point access
  __attribute__((noinline)) float getFloat(
      FloatField *field);
  __attribute__((noinline)) double getDouble(
      DoubleField *field);

  // Pointer access
  __attribute__((noinline)) Object *getObject(
      ObjectField *field);
  //  }
};

} // namespace objectir
