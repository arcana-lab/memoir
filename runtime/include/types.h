#ifndef MEMOIR_TYPES_H
#define MEMOIR_TYPES_H
#pragma once

/*
 * Object representation recognizable by LLVM IR
 * This file describes the Type interface for the
 * MemOIR library.
 *
 * Author(s): Tommy McMichen
 * Created: Mar 7, 2022
 */

#include <list>
#include <string>
#include <unordered_map>
#include <vector>

#include "objects.h"

namespace memoir {

struct Collection;

enum TypeCode {
  StructTy,
  TensorTy,
  AssocArrayTy,
  SequenceTy,
  IntegerTy,
  FloatTy,
  DoubleTy,
  PointerTy,
  ReferenceTy,
  NoneTy
};

struct Type {
public:
  TypeCode getCode();
  bool hasName();
  const char *getName();

  virtual bool equals(Type *other) = 0;
  virtual std::string to_string() = 0;

protected:
  Type(TypeCode code, const char *name);
  Type(TypeCode code);

private:
  TypeCode code;
  const char *name;
};

/*
 * Struct Type
 */
struct StructType : public Type {
public:
  static StructType *get(const char *name);
  static StructType *define(const char *name, std::vector<Type *> &field_types);

  std::vector<Type *> fields;

  bool equals(Type *other) override;
  std::string to_string() override;

private:
  StructType(const char *name, std::vector<Type *> &field_types);

  static std::unordered_map<const char *, Type *> &struct_types();
};

/*
 * Collection Types
 */
struct TensorType : public Type {
public:
  static TensorType *get(Type *element_type, uint64_t num_dimensions);
  static TensorType *get(Type *element_type,
                         uint64_t num_dimensions,
                         std::vector<uint64_t> &length_of_dimensions);

  static const uint64_t unknown_length;

  Type *element_type;
  uint64_t num_dimensions;
  bool is_static_length;
  std::vector<uint64_t> length_of_dimensions;

  bool equals(Type *other) override;
  std::string to_string() override;

private:
  TensorType(Type *element_type, uint64_t num_dimensions);
  TensorType(Type *element_type,
             uint64_t num_dimensions,
             std::vector<uint64_t> &length_of_dimensions);

  // clang-format off
  using static_tensor_type_list =
    std::list<
      std::tuple<
        Type *,                /* element type */
        uint64_t,              /* num dimensions */
        std::vector<uint64_t>, /* length of dimensions */
        TensorType *>>;
  using tensor_type_list =
    std::list<
      std::tuple<
        Type *,         /* element type */
        uint64_t,       /* num dimensions */
        TensorType *>>;
  // clang-format on

  static static_tensor_type_list &get_static_tensor_types();
  static tensor_type_list &get_tensor_types();
};

struct AssocArrayType : public Type {
public:
  static AssocArrayType *get(Type *key_type, Type *value_type);

  Type *key_type;
  Type *value_type;

  bool equals(Type *other) override;
  std::string to_string() override;

private:
  AssocArrayType(Type *key_type, Type *value_type);
};

struct SequenceType : public Type {
public:
  static SequenceType *get(Type *element_type);

  Type *element_type;

  bool equals(Type *other) override;
  std::string to_string() override;

private:
  SequenceType(Type *element_type);

  static std::unordered_map<Type *, SequenceType *> &sequence_types();
};

/*
 * Primitive Types
 */
struct IntegerType : public Type {
public:
  static IntegerType *get(unsigned bitwidth, bool is_signed);

  unsigned bitwidth;
  bool is_signed;

  bool equals(Type *other) override;
  std::string to_string() override;

private:
  IntegerType(unsigned bitwidth, bool is_signed);
};

struct FloatType : public Type {
public:
  static FloatType *get();

  bool equals(Type *other) override;
  std::string to_string() override;

private:
  FloatType();
};

struct DoubleType : public Type {
public:
  static DoubleType *get();

  bool equals(Type *other) override;
  std::string to_string() override;

private:
  DoubleType();
};

struct PointerType : public Type {
public:
  static PointerType *get();

  bool equals(Type *other) override;
  std::string to_string() override;

private:
  PointerType();
};

struct ReferenceType : public Type {
public:
  static ReferenceType *get(Type *referenced_type);

  Type *referenced_type;

  bool equals(Type *other) override;
  std::string to_string() override;

private:
  ReferenceType(Type *referenced_type);
};

/*
 * Helper functions
 */
bool is_object_type(Type *type);

bool is_struct_type(Type *type);

bool is_collection_type(Type *type);

bool is_intrinsic_type(Type *type);

} // namespace memoir

#endif
