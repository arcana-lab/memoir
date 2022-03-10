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

public:
  Field(Type *type);
  
  Type *getType();

  virtual std::string toString() = 0;
};

struct Object {
protected:
  Type *type;
  std::vector<Field *> fields;

  Object(Type *type);

public:
  // Access
  Field *readField(uint64_t fieldNo);
  void writeField(uint64_t fieldNo, Field *field);

  // Typing
  Type *getType();

  std::string toString();
};

struct Array : public Object {
protected:
  uint64_t length;

  Array(Type *t, uint64_t length);

public:
  // Access
  Field *getElement(uint64_t index);
  Field *setElement(uint64_t index);

  std::string toString();
};

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
};

struct FloatField : public Field {
protected:
  float value;

  FloatField(float init);

public:
  std::string toString();
};

struct DoubleField : public Field {
protected:
  double value;

  DoubleField(double init);

public:
  std::string toString();
};

struct PointerField : public Field {
protected:
  Object *value;

  PointerField(Object *obj);

public:
  std::string toString();
};
} // namespace objectir
