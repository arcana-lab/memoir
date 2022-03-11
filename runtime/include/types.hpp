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

struct Type {
protected:
  bool isObject;
  bool isArray;
  bool isInteger;
  bool isFloat;
  bool isDouble;

  Type();

public:
  virtual std::string toString() = 0;
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

  std::string toString();
};

struct FloatType : public Type {
  FloatType();

  std::string toString();
};

struct DoubleType : public Type {
  DoubleType();

  std::string toString();
};

} // namespace objectir
