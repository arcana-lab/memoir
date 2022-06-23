#ifndef MEMOIR_TYPES_H
#define MEMOIR_TYPES_H
#pragma once

/*
 * Object representation recognizable by LLVM IR
 * This file describes the Type interface for the
 * object-ir library.
 *
 * Author(s): Tommy McMichen
 * Created: Mar 7, 2022
 */

#include <string>
#include <unordered_map>
#include <vector>

namespace objectir {

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
protected:
  TypeCode code;
  std::string name;
  bool resolved;

public:
  TypeCode getCode();
  bool hasName();
  std::string getName();

  Type(TypeCode code, std::string name);
  Type(TypeCode code);
  ~Type();

  virtual Type *resolve() = 0;
  virtual bool equals(Type *other) = 0;
  virtual std::string toString() = 0;

  friend class Object;
  friend class Field;
};

struct StructType : public Type {
public:
  static StructType *get(std::vector<Type *> &field_types);

  std::vector<Type *> fields;

  Type *resolve();
  bool equals(Type *other);
  std::string toString();

private:
  StructType(std::string name);
  StructType();
  ~StructType();
};

struct TensorType : public Type {
public:
  static TensorType *get(Type *element_type, uint64_t num_dimensions);

  Type *element_type;
  uint64_t num_dimensions;

  Type *resolve();
  bool equals(Type *other);
  std::string toString();

private:
  TensorType(Type *elementType);
  ~TensorType();
};

struct IntegerType : public Type {
public:
  static IntegerType *get(unsigned bitwidth, bool is_signed);

  unsigned bitwidth;
  bool is_signed;

  Type *resolve();
  bool equals(Type *other);
  std::string toString();

private:
  IntegerType(unsigned bitwidth, bool is_signed);
  ~IntegerType();

  std::unordered_map<
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
  static FloatType *get();

  Type *resolve();
  bool equals(Type *other);
  std::string toString();

private:
  FloatType();
  ~FloatType();
};

struct DoubleType : public Type {
public:
  static DoubleType *get();

  Type *resolve();
  bool equals(Type *other);
  std::string toString();

private:
  DoubleType();
  ~DoubleType();
};

struct ReferenceType : public Type {
  Type *referenced_type;

  ReferenceType(Type *referenced_type);
  ~ReferenceType();

  Type *resolve();
  bool equals(Type *other);
  std::string toString();
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

  Type *resolvedType;

  StubType(std::string name);
  ~StubType();

  Type *resolve();
  bool equals(Type *other);

  std::string toString();
};

/*
 * Type Factory
 *
 * The Type factory allows the runtime to resolve
 * named types since they don't have a static view
 * of the program.
 */
class TypeFactory {
public:
  static TypeFactory *getInstance();

  void registerType(std::string name, Type *type);
  Type *getType(std::string name);

private:
  TypeFactory();

  std::unordered_map<std::string, Type *> nameToType;
};

/*
 * Helper functions
 */
bool isObjectType(Type *type);

bool isIntrinsicType(Type *type);

bool isStubType(Type *type);

} // namespace objectir

#endif
