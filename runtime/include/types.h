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

#include <string>
#include <unordered_map>
#include <vector>

namespace memoir {

enum TypeCode {
  StructTy,
  TensorTy,
  IntegerTy,
  FloatTy,
  DoubleTy,
  ReferenceTy,
  NoneTy
};

struct Type {
public:
  TypeCode getCode();
  bool hasName();
  const char *getName();

  virtual bool equals(Type *other) = 0;
  virtual std::string toString() = 0;

protected:
  Type(TypeCode code, const char *name);
  Type(TypeCode code);

private:
  TypeCode code;
  const char *name;
};

struct StructType : public Type {
public:
  static Type *get(const char *name);
  static Type *define(const char *name, std::vector<Type *> &field_types);

  std::vector<Type *> fields;

  bool equals(Type *other);
  std::string toString();

private:
  StructType(const char *name, std::vector<Type *> &field_types);

  static std::unordered_map<const char *, Type *> &struct_types();
};

struct TensorType : public Type {
public:
  static Type *get(Type *element_type, uint64_t num_dimensions);
  static Type *get(Type *element_type,
                   uint64_t num_dimensions,
                   std::vector<uint64_t> &length_of_dimensions);

  static const uint64_t unknown_length;

  Type *element_type;
  uint64_t num_dimensions;
  bool is_static_length;
  std::vector<uint64_t> length_of_dimensions;

  bool equals(Type *other);
  std::string toString();

private:
  TensorType(Type *element_type, uint64_t num_dimensions);
  TensorType(Type *element_type,
             uint64_t num_dimensions,
             std::vector<uint64_t> &length_of_dimensions);
};

struct IntegerType : public Type {
public:
  static Type *get(unsigned bitwidth, bool is_signed);

  unsigned bitwidth;
  bool is_signed;

  bool equals(Type *other);
  std::string toString();

private:
  IntegerType(unsigned bitwidth, bool is_signed);
};

struct FloatType : public Type {
public:
  static Type *get();

  bool equals(Type *other);
  std::string toString();

private:
  FloatType();
};

struct DoubleType : public Type {
public:
  static Type *get();

  bool equals(Type *other);
  std::string toString();

private:
  DoubleType();
};

struct ReferenceType : public Type {
public:
  static Type *get(Type *referenced_type);

  Type *referenced_type;

  bool equals(Type *other);
  std::string toString();

private:
  ReferenceType(Type *referenced_type);
};

/*
 * Helper functions
 */
bool isObjectType(Type *type);

bool isIntrinsicType(Type *type);

} // namespace memoir

#endif
