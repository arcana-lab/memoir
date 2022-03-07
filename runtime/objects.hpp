#pragma once

/*
 * Object representation recognizable by LLVM IR
 * This file describes the Object and Field representation
 * for the object-ir library
 *
 * Author(s): Tommy McMichen
 * Created: Mar 4, 2022
 */

#include "types.h"

namespace objectir {

struct Field {
  Type *type;

  Field(Type *type);

  Type *getType();

  virtual std::string toString() = 0;
};

struct IntegerField : public Field {
  uint64_t value;

  IntegerField(uint64_t init, uint64_t bitwidth, bool isSigned);
  IntegerField(uint8_t init);
  IntegerField(uint16_t init);
  IntegerField(uint32_t init);
  IntegerField(uint64_t init);
  IntegerField(int8_t init);
  IntegerField(int16_t init);
  IntegerField(int32_t init);
  IntegerField(int64_t init);

  std::string toString();
};

struct FloatField : public Field {
  float value;

  FloatField(float init);

  std::string toString();
};

struct DoubleField : public Field {
  double value;

  DoubleField(double init);

  std::string toString();
}

struct PointerField : public PointerField {
  Object *obj;

  PointerField(Object *obj);

  std::string toString();
};

struct Object {
  Type *type;
  std::vector<Field *> fields;

  Object(std::vector<Field *> fields);

  // Access
  Field *readField(uint64_t fieldNo);
  Field *writeField(uint64_t fieldNo);

  // Typing
  Type *getType();

  virtual std::string toString() = 0;
};

struct Array : public Object {
  uint64_t length;

  Array(uint64_t length);
  Array(uint64_t length, Field *init);

  // Access
  Field *getElement(uint64_t index);
  Field *setElement(uint64_t index);

  std::string toString();
};

} // namespace objectir
