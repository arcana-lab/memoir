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

#include "types.hpp"

namespace objectir {

struct Field {
protected:
  Type *type;

  Field(Type *type);

public:
  Type *getType();

  virtual std::string toString() = 0;

  friend class FieldAccessor;
};

struct Object {
protected:
  Type *type;
  std::vector<Field *> fields;

  // Construction
  Object(Type *type);
  ~Object();

public:
  // Access
  Field *readField(uint64_t fieldNo);
  void writeField(uint64_t fieldNo, Field *field);

  // Typing
  Type *getType();

  std::string toString();

  friend class ObjectBuilder;
};

struct Array : public Object {
protected:
  uint64_t length;

  // Construction
  Array(Type *t, uint64_t length);
  ~Array();

public:
  // Access
  Field *getElement(uint64_t index);
  Field *setElement(uint64_t index);

  std::string toString();

  friend class ObjectBuilder;
};

// Union
struct Union : public Object {
protected:
  std::vector<Field *> members;

  Union(Type *t);
  ~Union();

public:
  // Access
  Field *readField(uint64_t fieldNo);
  void writeField(uint64_t fieldNo, Field *field);

  // Typing
  Type *getType();

  std::string toString();

  friend class ObjectBuilder;
}

// Integer
struct IntegerField : public Field {
protected:
  uint64_t value;

  IntegerField(uint64_t init,
               uint64_t bitwidth,
               bool isSigned);
  IntegerField(uint8_t init);
  IntegerField(uint16_t init);
  IntegerField(uint32_t init);
  IntegerField(uint64_t init);
  IntegerField(int8_t init);
  IntegerField(int16_t init);
  IntegerField(int32_t init);
  IntegerField(int64_t init);

public:
  std::string toString();

  friend class ObjectBuilder;
};

// Floating point
struct FloatField : public Field {
protected:
  float value;

  FloatField(float init);

public:
  std::string toString();

  friend class ObjectBuilder;
};

// Double-precision floating point
struct DoubleField : public Field {
protected:
  double value;

  DoubleField(double init);

public:
  std::string toString();

  friend class ObjectBuilder;
};

// Indirect object
struct PointerField : public Field {
protected:
  Object *value;

  PointerField(Object *obj);

public:
  std::string toString();

  friend class ObjectBuilder;
};

// Nested object
struct ObjectField : public Field {
protected:
  Object *value;

  ObjectField(Object *obj);

public:
  std::string toString();

  friend class ObjectBuilder;
};

} // namespace objectir
