#pragma once

/*
 * Object representation recognizable by LLVM IR
 * This file contains methods to access object-ir
 * objects, fields and types for the objecy-ir library.
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
  extern "C" {
  __attribute__((noinline)) Field *readField(
      uint64_t fieldNo);
  __attribute__((noinline)) Field *writeField(
      uint64_t fieldNo,
      Field *field);
  }
}

/*
 * Array accesses
 */
class ArrayAccessor {
  extern "C" {
  __attribute__((noinline)) Field *readElement(
      uint64_t index);
  __attribute__((noinline)) Field *writeElement(
      uint64_t index,
      Field *element);
  }
}

/*
 * Union accesses
 */
class UnionAccessor {
  extern "C" {
  __attribute__((noinline)) Field *readMember(
      uint64_t index);
  __attribute__((noinline)) Field *writeMember(
      uint64_t index,
      Field *element);
  }
}

/*
 * Field accesses
 */
class FieldAccessor {
  extern "C" {
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
  __attribute__((noinline)) int8_t getInt8(IntegerField *field);

  // Floating point access
  __attribute__((noinline)) float getFloat(FloatField *field);
  __attribute__((noinline)) double getDouble(
      DoubleField *field);

  // Pointer access
  __attribute__((noinline)) Object *getObject(
      ObjectField *field);
  }
}

} // namespace objectir
