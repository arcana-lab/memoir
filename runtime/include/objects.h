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
#include <type_traits>
#include <vector>

#include "objects.h"
#include "types.h"

namespace memoir {

struct Element;
struct Type;
struct StructType;
struct TensorType;
struct AssocArrayType;
struct SequenceType;
struct IntegerType;
struct FloatType;
struct DoubleType;
struct PointerType;
struct ReferenceType;

struct Object {
public:
  // Owned state

  // Borrowed state
  Type *type;

  // Construction
  static Object *init(Type *type);
  Object(Type *type);

  // Access
  virtual Element *get_element(va_list args) = 0;
  virtual Object *get_slice(va_list args) = 0;
  virtual Object *join(va_list args, uint8_t num_args) = 0;
  virtual bool equals(const Object *other) const = 0;

  // Typing
  Type *get_type() const;
  virtual bool is_collection() const;
  virtual bool is_struct() const;
  virtual bool is_element() const;

  virtual std::string to_string() = 0;
};

struct Struct : public Object {
public:
  // Owned state
  std::vector<Element *> fields;

  // Borrowed state

  // Construction
  Struct(Type *type);
  Object *join(va_list args, uint8_t num_args) override;

  // Access
  Element *get_field(uint64_t field_index) const;
  Element *get_element(va_list args) override;
  Object *get_slice(va_list args) override;

  bool is_struct() const override;
  bool equals(const Object *other) const override;

  // Debug
  std::string to_string() override;
};

struct Collection : public Object {
public:
  // Access
  virtual Type *get_element_type() const = 0;
  bool is_collection() const override;

  // Construction
  Collection(Type *type);
};

struct Tensor : public Collection {
public:
  // Owned state
  std::vector<Element *> tensor;
  std::vector<uint64_t> length_of_dimensions;

  // Borrowed state

  // Construction
  Tensor(Type *type);
  Tensor(Type *type, std::vector<uint64_t> &length_of_dimensions);

  // Access
  Element *get_tensor_element(std::vector<uint64_t> &indices) const;
  Element *get_element(va_list args) override;
  Object *get_slice(va_list args) override;
  Object *join(va_list args, uint8_t num_args) override;
  Type *get_element_type() const override;

  bool equals(const Object *other) const override;

  // Debug
  std::string to_string() override;
};

struct AssocArray : public Collection {
public:
  using key_t = std::add_pointer<Object>::type;
  using value_t = std::add_pointer<Element>::type;
  using key_value_pair_t = std::unordered_map<key_t, value_t>::value_type;

  // Owned state
  std::unordered_map<key_t, value_t> assoc_array;

  // Borrowed state

  // Construction
  AssocArray(Type *type);
  Object *join(va_list args, uint8_t num_args) override;

  // Access
  Element *get_element(va_list args) override;
  Object *get_slice(va_list args) override;
  AssocArray::key_value_pair_t &get_pair(Object *key);
  Type *get_element_type() const override;
  Type *get_key_type() const;
  Type *get_value_type() const;

  bool equals(const Object *other) const override;

  // Debug
  std::string to_string() override;
};

struct Sequence : public Collection {
public:
  // Owned state
  std::vector<Element *> sequence;

  // Construction
  Sequence(Type *type, uint64_t init_size);
  Object *join(va_list args, uint8_t num_args) override;
  static Object *join(SequenceType *type,
                      std::vector<Sequence *> sequences_to_join);

  // Access
  Element *get_element(va_list args) override;
  Element *get_element(uint64_t index);
  Object *get_slice(va_list args) override;
  Object *get_slice(int64_t left_index, int64_t right_index);
  Type *get_element_type() const override;

  bool equals(const Object *other) const override;

  // Debug
  std::string to_string() override;
};

// Abstract Element
struct Element : public Object {
public:
  // Construction
  static Element *create(Type *type);
  virtual Element *clone() const = 0;
  Object *join(va_list args, uint8_t num_args) override;

  // Access
  Element *get_element(va_list args) override;
  Object *get_slice(va_list args) override;

  bool is_element() const override;

protected:
  Element(Type *type);
};

// Integer
struct IntegerElement : public Element {
public:
  uint64_t value;

  uint64_t read_value() const;
  void write_value(uint64_t value);
  bool equals(const Object *other) const override;

  // Construction
  IntegerElement(IntegerType *type);
  IntegerElement(IntegerType *type, uint64_t init);
  Element *clone() const override;

  // Debug
  std::string to_string() override;
};

// Floating point
struct FloatElement : public Element {
public:
  float value;

  float read_value() const;
  void write_value(float value);
  bool equals(const Object *other) const override;

  // Construction
  FloatElement(FloatType *type);
  FloatElement(FloatType *type, float init);
  Element *clone() const override;

  // Debug
  std::string to_string() override;
};

// Double-precision floating point
struct DoubleElement : public Element {
public:
  double value;

  double read_value() const;
  void write_value(double value);
  bool equals(const Object *other) const override;

  // Construction
  DoubleElement(DoubleType *type);
  DoubleElement(DoubleType *type, double init);
  Element *clone() const override;

  // Debug
  std::string to_string() override;
};

// Pointer to non-memoir memory
struct PointerElement : public Element {
public:
  void *value;

  // Access
  void *read_value() const;
  void write_value(void *value);
  bool equals(const Object *other) const override;

  // Construction
  PointerElement(PointerType *type);
  PointerElement(PointerType *type, void *init);
  Element *clone() const override;

  // Debug
  std::string to_string() override;
};

// Reference to memoir object
struct ReferenceElement : public Element {
public:
  Object *value;

  Object *read_value() const;
  void write_value(Object *value);
  bool equals(const Object *other) const override;

  // Construction
  ReferenceElement(ReferenceType *type);
  ReferenceElement(ReferenceType *type, Object *init);
  Element *clone() const override;

  // Debug
  std::string to_string() override;
};

// Nested object
struct ObjectElement : public Element {
public:
  // Access
  virtual Object *read_value() const = 0;
  bool equals(const Object *other) const override;

  // Construction
  ObjectElement(Type *type);
};

struct StructElement : public ObjectElement {
public:
  Struct *value;

  // Access
  Object *read_value() const override;

  // Construction
  StructElement(StructType *type);
  StructElement(StructType *type, Struct *init);
  Element *clone() const override;

  // Debug
  std::string to_string() override;
};

struct TensorElement : public ObjectElement {
public:
  Tensor *value;

  // Access
  Object *read_value() const override;

  // Construction
  TensorElement(TensorType *type);
  TensorElement(TensorType *type, Tensor *init);
  Element *clone() const override;

  // Debug
  std::string to_string() override;
};

} // namespace memoir

#endif
