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

namespace memoir {

struct Object {
public:
  Type *type;

  static Object *init(Type *type);

  // Construction
  Object(Type *type);
  ~Object();

  // Typing
  Type *getType();

  std::string toString();
};

struct Field : public Object {
public:
  Field(Type *type);

  static Field *createField(Type *type);

  virtual std::string toString() = 0;
};

struct Struct : public Object {
public:
  std::vector<Field *> fields;

  // Construction
  Struct(Type *type);

  // Access
  Field *readField(uint64_t field_index);
  void writeField(uint64_t field_index, Field *field);

  std::string toString();
};

struct Tensor : public Object {
public:
  std::vector<Field *> fields;
  std::vector<uint64_t> length_of_dimensions;

  // Construction
  Tensor(Type *type);
  Tensor(Type *type, std::vector<uint64_t> &length_of_dimensions);

  // Access
  Field *getElement(std::vector<uint64_t> &indices);

  std::string toString();
};

// Integer
struct IntegerField : public Field {
public:
  uint64_t value;

  uint64_t readValue();
  void writeValue(uint64_t value);

  // Construction
  IntegerField(Type *type);
  IntegerField(Type *type, uint64_t init);

  std::string toString();
};

// Floating point
struct FloatField : public Field {
public:
  float value;

  float readValue();
  void writeValue(float value);

  // Construction
  FloatField(Type *type);
  FloatField(Type *type, float init);

  std::string toString();
};

// Double-precision floating point
struct DoubleField : public Field {
public:
  double value;

  double readValue();
  void writeValue(double value);

  // Construction
  DoubleField(Type *type);
  DoubleField(Type *type, double init);

  std::string toString();
};

// Nested object
struct StructField : public Field {
public:
  Struct *value;

  Struct *readValue();

  // Construction
  StructField(Type *type);
  StructField(Type *type, Struct *init);

  std::string toString();
};

// Nested tensor
struct TensorField : public Field {
public:
  Tensor *value;

  Tensor *readValue();

  // Construction
  TensorField(Type *type);
  TensorField(Type *type, Tensor *init);

  std::string toString();
};

// Indirection pointer
struct ReferenceField : public Field {
public:
  Object *value;

  Object *readValue();
  void writeValue(Object *value);

  // Construction
  ReferenceField(Type *type);
  ReferenceField(Type *type, Object *init);

  std::string toString();
};

} // namespace memoir

#endif
