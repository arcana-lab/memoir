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
  StubTy,
};

struct Type {
public:
  TypeCode getCode();
  bool hasName();
  std::string getName();

  static Type *find(std::string name);
  static void define(std::string name, Type *type_to_define);

  virtual Type *resolve() = 0;
  virtual bool equals(Type *other) = 0;
  virtual std::string toString() = 0;

protected:
  Type(TypeCode code, std::string name);
  Type(TypeCode code);

private:
  TypeCode code;
  std::string name;

  static std::unordered_map<std::string, Type *> named_types;
};

struct StructType : public Type {
public:
  static Type *get(std::string name, std::vector<Type *> &field_types);

  std::vector<Type *> fields;

  Type *resolve();
  bool equals(Type *other);
  std::string toString();

private:
  StructType(std::string name, std::vector<Type *> &field_types);
};

struct TensorType : public Type {
public:
  static Type *get(Type *element_type, uint64_t num_dimensions);

  Type *element_type;
  uint64_t num_dimensions;

  Type *resolve();
  bool equals(Type *other);
  std::string toString();

private:
  TensorType(Type *elementType, uint64_t num_dimensions);
};

struct IntegerType : public Type {
public:
  static Type *get(unsigned bitwidth, bool is_signed);

  unsigned bitwidth;
  bool is_signed;

  Type *resolve();
  bool equals(Type *other);
  std::string toString();

private:
  IntegerType(unsigned bitwidth, bool is_signed);

  static std::unordered_map<
      // bitwidth
      unsigned,
      std::unordered_map<
          // is_signed
          bool,
          IntegerType *>>
      integer_types;
};

struct FloatType : public Type {
public:
  static Type *get();

  Type *resolve();
  bool equals(Type *other);
  std::string toString();

private:
  FloatType();
};

struct DoubleType : public Type {
public:
  static Type *get();

  Type *resolve();
  bool equals(Type *other);
  std::string toString();

private:
  DoubleType();
};

struct ReferenceType : public Type {
public:
  static Type *get(Type *referenced_type);

  Type *referenced_type;

  Type *resolve();
  bool equals(Type *other);
  std::string toString();

private:
  ReferenceType(Type *referenced_type);
};

/*
 * Stub Type
 *
 * The stub type is used to represent a named type that
 * hasn't been defined yet. Using a field of stub type
 * before the stub type is resolved will result in an error.
 *
 * NOTE: The stub type does not need to be represented in
 * the middle end, it is only necessary to make the runtime
 * work.
 */
struct StubType : public Type {
public:
  static Type *get(std::string name);

  std::string name;

  Type *resolved_type;

  Type *resolve();
  bool equals(Type *other);

  std::string toString();

private:
  StubType(std::string name);
  ~StubType();

  static std::unordered_map<std::string, Type *> stub_types;
};

/*
 * Helper functions
 */
bool isObjectType(Type *type);

bool isIntrinsicType(Type *type);

bool isStubType(Type *type);

} // namespace memoir

#endif
