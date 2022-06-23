#ifndef MEMOIR_OBJECTS_H
#define MEMOIR_OBJECTS_H
#pragma once

/*
 * Object representation recognizable by LLVM IR
 * This file describes the Object and Field representation
 * for the object-ir library
 *
 * Author(s): Tommy McMichen
 * Created: Mar 4, 2022
 */

#include <cstdint>
#include <string>
#include <vector>

#include "objects.h"
#include "types.h"

#ifdef __cplusplus
namespace memoir {
#endif

struct Object {
public:
  Type *type;

  // Construction
  Object(Type *type);
  ~Object();

  // Typing
  Type *getType();

  std::string toString();
};

struct Struct : public Object {
public:
  std::vector<Field *> fields;

  // Access
  Field *readField(uint64_t field_index);
  void writeField(uint64_t field_index, Field *field);

  std::string toString();
}

struct Tensor : public Object {
public:
  std::vector<uint64_t> length_of_dimensions;

  // Construction
  Tensor(Type *t, std::vector<uint64_t> &length_of_dimensions);

  // Access
  Field *getElement(std::vector<uint64_t> &indices);

  std::string toString();
};

template <class T>
struct Field : public Object {
public:
  T value;

  T readValue();
  void writeValue(T value);

  Field(Type *type);
  Field(Type *type, T init);

  static Field *createField(Type *type);

  virtual std::string toString() = 0;
};

// Integer
struct IntegerField : public Field<uint64_t> {
public:
  // Construction
  IntegerField(uint64_t bitwidth, bool isSigned);
  IntegerField(uint64_t bitwidth, bool isSigned, uint64_t init);

  std::string toString();
};

// Floating point
struct FloatField : public Field<float> {
public:
  // Construction
  FloatField();
  FloatField(float init);

  std::string toString();
};

// Double-precision floating point
struct DoubleField : public Field<double> {
public:
  // Construction
  DoubleField();
  DoubleField(double init);

  std::string toString();
};

// Nested object
struct ObjectField : public Field<Object *> {
public:
  // Construction
  ObjectField(Type *type);
  ObjectField(Object *init);

  std::string toString();
};

// Indirection pointer
struct ReferenceField : public Field<Object *> {
public:
  // Construction
  ReferenceField(Type *type);

  std::string toString();
};

#ifdef __cplusplus
} // namespace objectir
#endif

#endif
