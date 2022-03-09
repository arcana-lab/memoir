#pragma once

/*
 * Object representation recognizable by LLVM IR
 * This file describes the Type interface for the
 * object-ir library.
 *
 * Author(s): Tommy McMichen
 * Created: Mar 7, 2022
 */

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

  std::string toString();
};

struct IntegerType : public Type {
  uint64_t bitwidth;
  bool isSigned;

  IntegerType(uint64_t bitwidth, bool isSigned);

  std::string toString();
};

struct FloatType : public Type {
  std::string toString();
};

struct DoubleType : public Type {
  std::string toString();
};

} // namespace objectir
