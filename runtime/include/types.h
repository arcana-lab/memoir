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
  ObjectTy,
  ArrayTy,
  UnionTy,
  IntegerTy,
  FloatTy,
  DoubleTy,
  PointerTy,
  StubTy,
};

struct Type {
protected:
  TypeCode code;
  std::string name;

public:
  TypeCode getCode();
  bool hasName();
  std::string getName();

  Type(TypeCode code, std::string name);
  Type(TypeCode code);
  ~Type();

  virtual std::string toString() = 0;

  friend class Object;
  friend class Field;
};

struct ObjectType : public Type {
  std::vector<Type *> fields;

  ObjectType(std::string name);
  ObjectType();
  ~ObjectType();

  std::string toString();
};

struct ArrayType : public Type {
  Type *elementType;

  ArrayType(Type *elementType, std::string name);
  ArrayType(Type *elementType);
  ~ArrayType();

  std::string toString();
};

struct UnionType : public Type {
  std::vector<Type *> members;

  UnionType(std::string name);
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
  std::string name;

  Type *resolvedType;

  StubType(std::string name);
  ~StubType();

  Type *resolve();

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

} // namespace objectir
