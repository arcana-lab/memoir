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

#include "objects.hpp"
#include "types.hpp"

namespace objectir {

struct Field {
protected:
  Type *type;

  Field(Type *type);

public:
  Type *getType();

  static Field *createField(Type *type);

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
  friend class ObjectAccessor;
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
  friend class ArrayAccessor;
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
  friend class UnionAccessor;
};

// Integer
struct IntegerField : public Field {
protected:
  uint64_t value;

  IntegerField(Type *t);
  IntegerField(Type *t, uint64_t init);
  IntegerField(uint64_t init,
               uint64_t bitwidth,
               bool isSigned);

public:
  std::string toString();

  friend class ObjectBuilder;
  friend class Field;
  friend class FieldAccessor;
};

// Floating point
struct FloatField : public Field {
protected:
  float value;

  FloatField(Type *type);
  FloatField(Type *type, float init);

public:
  std::string toString();

  friend class ObjectBuilder;
  friend class Field;
  friend class FieldAccessor;
};

// Double-precision floating point
struct DoubleField : public Field {
protected:
  double value;

  DoubleField(Type *type);
  DoubleField(Type *type, double init);

public:
  std::string toString();

  friend class ObjectBuilder;
  friend class Field;
  friend class FieldAccessor;
};

// Nested object
struct ObjectField : public Field {
protected:
  Object *value;

  ObjectField(Type *type);
  ObjectField(Object *init);

public:
  std::string toString();

  friend class ObjectBuilder;
  friend class Field;
  friend class FieldAccessor;
};

} // namespace objectir
