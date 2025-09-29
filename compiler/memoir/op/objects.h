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

#include <map>
#include <memory>
#include <stdint.h>
#include <string>
#include <type_traits>
#include <vector>

#include "objects.h"
#include "types.h"

namespace memoir {

struct Type;
struct StructType;
struct AssocArrayType;
struct SequenceType;
struct IntegerType;
struct FloatType;
struct DoubleType;
struct PointerType;
struct ReferenceType;

namespace detail {
struct Object {
public:
  // Owned state

  // Borrowed state
  Type *type;

  // Construction
  static Object *init(Type *type);
  Object(Type *type);
  virtual void free() = 0;

  // Access
  virtual bool equals(const Object *other) const = 0;

  // Typing
  Type *get_type() const;
  virtual bool is_collection() const;
  virtual bool is_struct() const;
  virtual bool is_element() const;

  virtual std::string to_string() = 0;
};
} // namespace detail

namespace detail {
struct Struct : public detail::Object {
public:
  // Owned state
  std::vector<uint64_t> fields;

  // Borrowed state

  // Construction
  Struct(Type *type);
  Struct(Struct *other);
  void free() override;
  ~Struct() = default;

  // Access
  uint64_t get_field(uint64_t field_index) const;
  void set_field(uint64_t value, uint64_t field_index);
  bool is_struct() const override;
  bool equals(const Object *other) const override;

  // Debug
  std::string to_string() override;
};
} // namespace detail

namespace detail {
struct Collection : public detail::Object {
public:
  // Access
  virtual uint64_t get_element(va_list args) = 0;
  virtual void set_element(uint64_t value, va_list args) = 0;
  virtual bool has_element(va_list args) = 0;
  virtual void remove_element(va_list args) = 0;
  virtual Collection *get_slice(va_list args) = 0;
  virtual Collection *join(va_list args, uint8_t num_args) = 0;
  virtual Type *get_element_type() const = 0;
  bool is_collection() const override;
  virtual uint64_t size() const = 0;
  virtual void clear() = 0;

  // Construction
  Collection(Type *type);
};

struct AssocArray : public detail::Collection {
public:
  using key_t = uint64_t;
  using value_t = uint64_t;

  // Owned state
  std::unordered_map<key_t, value_t> assoc_array;

  // Borrowed state

  // Construction
  AssocArray(Type *type);
  ~AssocArray() = default;
  void free() override;

  // Operations
  Collection *join(va_list args, uint8_t num_args) override;
  Collection *get_slice(va_list args) override;
  uint64_t size() const override;
  Collection *keys() const;
  void clear() override;

  // Access
  uint64_t get_element(va_list args) override;
  void set_element(uint64_t value, va_list args) override;
  bool has_element(va_list args) override;
  void remove_element(va_list args) override;
  Type *get_element_type() const override;
  Type *get_key_type() const;
  Type *get_value_type() const;

  bool equals(const Object *other) const override;

  // Debug
  std::string to_string() override;
};

struct Sequence : public detail::Collection {
protected:
  using seq_iter = std::vector<uint64_t>::iterator;
  using const_seq_iter = std::vector<uint64_t>::const_iterator;

public:
  // Construction
  Sequence(SequenceType *type);

  // Operations
  static Collection *join(SequenceType *type,
                          std::vector<Sequence *> sequences_to_join);
  Collection *join(va_list args, uint8_t num_args) override;
  virtual Collection *get_sequence_slice(int64_t left_index,
                                         int64_t right_index) = 0;
  Collection *get_slice(va_list args) override;
  void clear() override;

  // Access
  virtual uint64_t get_sequence_element(uint64_t index) = 0;
  uint64_t get_element(va_list args) override;
  virtual void set_sequence_element(uint64_t value, uint64_t index) = 0;
  void set_element(uint64_t value, va_list args) override;
  virtual bool has_sequence_element(uint64_t index) = 0;
  bool has_element(va_list args) override;
  void remove_element(va_list args) override;

  Type *get_element_type() const override;

  // Iterators
  virtual seq_iter begin() = 0;
  virtual seq_iter end() = 0;
  virtual const_seq_iter cbegin() const = 0;
  virtual const_seq_iter cend() const = 0;

  // Mutable operations
  virtual void insert(uint64_t index, uint64_t value) = 0;
  virtual void insert(uint64_t start, Sequence *seq) = 0;
  virtual void erase(uint64_t from, uint64_t to) = 0;
  virtual void grow(uint64_t size) = 0;
};

struct SequenceAlloc : public detail::Sequence {
  // Owned state
  std::vector<uint64_t> _sequence;

  // Construction
  SequenceAlloc(SequenceType *type, size_t initial_size);
  SequenceAlloc(SequenceType *type, std::vector<uint64_t> &&initial);
  ~SequenceAlloc() = default;
  void free() override;

  // Operations
  Collection *get_sequence_slice(int64_t left_index,
                                 int64_t right_index) override;

  // Access
  uint64_t get_sequence_element(uint64_t index) override;
  void set_sequence_element(uint64_t value, uint64_t index) override;
  bool has_sequence_element(uint64_t index) override;

  uint64_t size() const override;

  bool equals(const Object *other) const override;

  // Iterators
  Sequence::seq_iter begin() override;
  Sequence::seq_iter end() override;
  Sequence::const_seq_iter cbegin() const override;
  Sequence::const_seq_iter cend() const override;

  // Mutable operations
  void insert(uint64_t index, uint64_t value) override;
  void insert(uint64_t start, Sequence *seq) override;
  void erase(uint64_t from, uint64_t to) override;
  void grow(uint64_t size) override;

  // Debug
  std::string to_string() override;
};

struct SequenceView : public detail::Sequence {
  // Borrowed state
  SequenceAlloc *_sequence;

  // Owned state
  size_t from;
  size_t to;

  // Construction
  SequenceView(SequenceAlloc *seq, size_t from, size_t to);
  SequenceView(SequenceView *view, size_t from, size_t to);
  ~SequenceView() = default;
  void free() override;

  // Operations
  Collection *get_sequence_slice(int64_t left_index,
                                 int64_t right_index) override;

  // Access
  uint64_t get_sequence_element(uint64_t index) override;
  void set_sequence_element(uint64_t value, uint64_t index) override;
  bool has_sequence_element(uint64_t index) override;
  uint64_t size() const override;

  // Iterators
  Sequence::seq_iter begin() override;
  Sequence::seq_iter end() override;
  Sequence::const_seq_iter cbegin() const override;
  Sequence::const_seq_iter cend() const override;

  // Mutable operations
  void insert(uint64_t index, uint64_t value) override;
  void insert(uint64_t start, Sequence *seq) override;
  void erase(uint64_t from, uint64_t to) override;
  void grow(uint64_t size) override;

  // Debug
  bool equals(const Object *other) const override;
  std::string to_string() override;
};

uint64_t init_element(Type *type);
std::vector<uint64_t> init_elements(Type *type, size_t num = 1);

} // namespace detail

struct Struct {
  Struct(detail::Struct *storage) : _storage(storage) {}

  operator detail::Struct *() const {
    return this->_storage;
  }

  detail::Struct *_storage;
};
struct Collection {
  Collection(detail::Collection *storage) : _storage(storage) {}

  operator detail::Collection *() const {
    return this->_storage;
  }

  detail::Collection *_storage;
};

} // namespace memoir

#endif
