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
namespace objectir {
#endif

struct Field {
public:
  Type *type;
  
  Type *getType();

  Field(Type *type);

  static Field *createField(Type *type);

  virtual std::string toString() = 0;
};

struct Object {
public:
  Type *type;
  std::vector<Field *> fields;

  // Construction
  Object(Type *type);
  ~Object();

  // Access
  Field *readField(uint64_t fieldNo);
  void writeField(uint64_t fieldNo, Field *field);

  // Typing
  Type *getType();

  std::string toString();
};

struct Array : public Object {
public:
  uint64_t length;
  
  // Construction
  Array(Type *t, uint64_t length);

  // Access
  Field *getElement(uint64_t index);
  Field *setElement(uint64_t index);

  std::string toString();
};

// Union
struct Union : public Object {
public:
  std::vector<Field *> members;
  
  // Construction
  Union(Type *t);
  ~Union();

  // Access
  Field *readField(uint64_t fieldNo);
  void writeField(uint64_t fieldNo, Field *field);

  // Typing
  Type *getType();

  std::string toString();

};

// Integer
struct IntegerField : public Field {
public:
  uint64_t value;
  
  // Construction
  IntegerField(Type *t);
  IntegerField(Type *t, uint64_t init);
  IntegerField(uint64_t init,
               uint64_t bitwidth,
               bool isSigned);

  // Access
  uint64_t readValue();
  void writeValue(uint64_t);

  std::string toString();
};

// Floating point
struct FloatField : public Field {
public:
  float value;
  
  // Construction
  FloatField(Type *type);
  FloatField(Type *type, float init);

  // Access
  float readValue();
  void writeValue(float value);

  std::string toString();
};

// Double-precision floating point
struct DoubleField : public Field {
public:
  double value;
  
  // Construction
  DoubleField(Type *type);
  DoubleField(Type *type, double init);

  // Access
  double readField();
  void writeField(double value);

  std::string toString();
};

// Nested object
struct ObjectField : public Field {
public:
  Object *value;

  // Construction
  ObjectField(Type *type);
  ObjectField(Object *init);

  std::string toString();
};

#ifdef __cplusplus
} // namespace objectir
#endif
