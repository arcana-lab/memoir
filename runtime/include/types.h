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
#include <vector>

namespace objectir {

enum TypeCode {
  ObjectTy,
  ArrayTy,
  UnionTy,
  IntegerTy,
  FloatTy,
  DoubleTy,
  PointerTy,
};

struct Type {
protected:
  TypeCode code;

public:
  TypeCode getCode();

  Type(TypeCode code);
  ~Type();

  virtual std::string toString() = 0;

  friend class Object;
  friend class Field;
};

struct ObjectType : public Type {
  std::vector<Type *> fields;

  ObjectType();
  ~ObjectType();

  std::string toString();
};

struct ArrayType : public Type {
  Type *elementType;

  ArrayType(Type *elementType);
  ~ArrayType();

  std::string toString();
};

struct UnionType : public Type {
  std::vector<Type *> members;

  UnionType();
  ~UnionType();

  std::string toString();
};

struct IntegerType : public Type {
  uint64_t bitwidth;
  bool isSigned;

  IntegerType(uint64_t bitwidth, bool isSigned);
  ~IntegerType();

  std::string toString();
};

struct FloatType : public Type {
  FloatType();
  ~FloatType();

  std::string toString();
};

struct DoubleType : public Type {
  DoubleType();
  ~DoubleType();

  std::string toString();
};

struct PointerType : public Type {
  Type *containedType;

  PointerType(Type *containedType);
  ~PointerType();

  std::string toString();
};

/*
 * Helper functions
 */
bool isObjectType(Type *type);

bool isIntrinsicType(Type *type);

} // namespace objectir
