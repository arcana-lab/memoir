#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H
#pragma once

#include <string>
#include <unordered_map>

#include "common/utility/FunctionNames.hpp"

/*
 * A simple analysis to summarize the Types present in a program.
 *
 * Author(s): Tommy McMichen
 * Created: July 5, 2022
 */

namespace llvm::memoir {

class TypeSummary;

class TypeAnalysis {
public:
  TypeAnalysis(llvm::Module &M);

  TypeSummary *getTypeSummary(llvm::CallInst &call_inst);

  /*
   * Helper functions
   */
  static bool isObjectType(TypeSummary *type);

  static bool isPrimitiveType(TypeSummary *type);

  static bool isStubType(TypeSummary *type);

private:
  Module &M;

  std::unordered_map<llvm::CallInst *, TypeSummary *> type_summaries;

  TypeSummary *getPrimitiveTypeSummary(llvm::CallInst &call_inst);

  TypeSummary *getIntegerTypeSummary(llvm::CallInst &call_inst);

  TypeSummary *getFloatTypeSummary(llvm::CallInst &call_inst);

  TypeSummary *getDoubleTypeSummary(llvm::CallInst &call_inst);

  TypeSummary *getReferenceTypeSummary(llvm::CallInst &call_inst);

  TypeSummary *getStructTypeSummary(llvm::CallInst &call_inst);

  TypeSummary *getTensorTypeSummary(llvm::CallInst &call_inst);

  TypeSummary *defineStructTypeSummary(llvm::CallInst &call_inst);
};

enum TypeCode {
  StructTy,
  TensorTy,
  IntegerTy,
  FloatTy,
  DoubleTy,
  ReferenceTy,
};

struct TypeSummary {
public:
  TypeCode getCode();

  virtual bool equals(Type *other) = 0;
  virtual std::string toString() = 0;

private:
  TypeCode code;

  Type(TypeCode code, std::string name);
  Type(TypeCode code);
};

struct StructTypeSummary : public TypeSummary {
public:
  static TypeSummary *get(std::string name);
  static TypeSummary *get(std::string name,
                          std::vector<TypeSummary *> &field_types);

  std::string getName();
  TypeSummary *getField(uint64_t field_index);
  uint64_t getNumFields();

  bool equals(TypeSummary *other);
  std::string toString();

private:
  static std::unordered_map<std::string name, StructTypeSummary *>
      defined_type_summaries;

  std::string name;
  std::vector<TypeSummary *> field_types;

  StructTypeSummary(std::string name, std::vector<TypeSummary *> &field_types);
};

struct TensorTypeSummary : public TypeSummary {
public:
  static TypeSummary *get(TypeSummary *element_type, uint64_t num_dimensions);
  static TypeSummary *get(TypeSummary *element_type,
                          std::vector<uint64_t> &length_of_dimensions);

  TypeSummary *getElementType();
  uint64_t getNumDimensions();
  bool isStaticLength();
  uint64_t getLengthOfDimension(uint64_t dimension_index);

  bool equals(Type *other);
  std::string toString();

private:
  TypeSummary *element_type;
  uint64_t num_dimensions;
  bool is_static_length;
  std::vector<uint64_t> length_of_dimensions;

  TensorTypeSummary(Type *element_type, uint64_t num_dimensions);
  TensorTypeSummary(Type *element_type,
                    std::vector<uint64_t> &length_of_dimensions);
};

struct ReferenceTypeSummary : public TypeSummary {
public:
  static TypeSummary *get(TypeSummary *referenced_type);

  TypeSummary *getReferencedType();

  bool equals(TypeSummary *other);
  std::string toString();

private:
  TypeSummary *referenced_type;

  ReferenceTypeSummary(TypeSummary *referenced_type);
};

struct IntegerTypeSummary : public TypeSummary {
public:
  static TypeSummary *get(unsigned bitwidth, bool is_signed);

  unsigned getBitWidth();
  bool isSigned();

  bool equals(TypeSummary *other);
  std::string toString();

private:
  unsigned bitwidth;
  bool is_signed;

  static std::unordered_map<unsigned,                /* bitwidth */
                            std::unordered_map<bool, /* is signed? */
                                               TypeSummary *>>
      integer_type_summaries;

  IntegerTypeSummary(unsigned bitwidth, bool is_signed);
};

struct FloatTypeSummary : public TypeSummary {
public:
  static TypeSummary *get();

  bool equals(TypeSummary *other);
  std::string toString();

private:
  FloatTypeSummary();
};

struct DoubleTypeSummary : public TypeSummary {
public:
  static TypeSummary *get();

  bool equals(TypeSummary *other);
  std::string toString();

private:
  DoubleTypeSummary();
};

} // namespace llvm::memoir

#endif // COMMON_TYPES_H
