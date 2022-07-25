#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H
#pragma once

#include <iostream>
#include <string>

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"

#include "common/support/InternalDatatypes.hpp"
#include "common/utility/FunctionNames.hpp"

/*
 * A simple analysis to summarize the Types present in a program.
 *
 * Author(s): Tommy McMichen
 * Created: July 5, 2022
 */

namespace llvm::memoir {

class TypeSummary;

/*
 * Type Analysis
 *
 * Top level entry for MemOIR type analysis
 *
 * This type analysis provides basic information about MemOIR
 *   types defined in the program.
 */
class TypeAnalysis {
public:
  /*
   * Singleton access
   */
  static TypeAnalysis &get(Module &M);

  static void invalidate(Module &M);

  /*
   * Query the Type Summary for the given LLVM Value
   */
  TypeSummary *getTypeSummary(llvm::Value &value);

  /*
   * Helper functions
   */

  /*
   * This class is not cloneable nor assignable
   */
  TypeAnalysis(TypeAnalysis &other) = delete;
  void operator=(const TypeAnalysis &) = delete;

private:
  /*
   * Passed state
   */
  Module &M;

  /*
   * Internal state
   */
  map<llvm::Value *, TypeSummary *> type_summaries;

  /*
   * Internal helper functions
   */
  TypeSummary *getMemOIRTypeSummary(llvm::CallInst &call_inst);

  TypeSummary &getPrimitiveTypeSummary(MemOIR_Func function_enum);

  TypeSummary &getIntegerTypeSummary(llvm::CallInst &call_inst);

  TypeSummary &getReferenceTypeSummary(llvm::CallInst &call_inst);

  TypeSummary &getStructTypeSummary(llvm::CallInst &call_inst);

  TypeSummary &getTensorTypeSummary(llvm::CallInst &call_inst);

  TypeSummary &defineStructTypeSummary(llvm::CallInst &call_inst);

  /*
   * Private constructor and logistics
   */
  TypeAnalysis(llvm::Module &M);

  void invalidate();

  static map<llvm::Module *, TypeAnalysis *> analyses;
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

  // TODO: change this to be an operator== override
  bool equals(TypeSummary *other);
  virtual std::string toString(std::string indent = "") const = 0;

  friend std::ostream &operator<<(std::ostream &os, const TypeSummary &ts);
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                       const TypeSummary &ts);
  friend bool operator<(const TypeSummary &l, const TypeSummary &r);

protected:
  TypeCode code;

  TypeSummary(TypeCode code);

  friend class TypeAnalysis;
};

struct StructTypeSummary : public TypeSummary {
public:
  static StructTypeSummary &get(std::string name);
  static StructTypeSummary &get(std::string name,
                                std::vector<TypeSummary *> &field_types);

  std::string getName();
  TypeSummary &getField(uint64_t field_index);
  uint64_t getNumFields();

  std::string toString(std::string indent = "") const override;

protected:
  std::string name;
  std::vector<TypeSummary *> field_types;

  static map<std::string, StructTypeSummary *> defined_type_summaries;

  StructTypeSummary(std::string name, std::vector<TypeSummary *> &field_types);

  friend class TypeAnalysis;
};

struct TensorTypeSummary : public TypeSummary {
public:
  static TensorTypeSummary &get(TypeSummary &element_type,
                                uint64_t num_dimensions);
  // static TensorTypeSummary &get(TypeSummary &element_type,
  //                               std::vector<uint64_t> &length_of_dimensions);

  TypeSummary &getElementType();
  uint64_t getNumDimensions();
  bool isStaticLength();
  uint64_t getLengthOfDimension(uint64_t dimension_index);

  std::string toString(std::string indent = "") const override;

protected:
  TypeSummary &element_type;
  uint64_t num_dimensions;
  bool is_static_length;
  std::vector<uint64_t> length_of_dimensions;

  static map<TypeSummary *, map<uint64_t, TensorTypeSummary *>>
      tensor_type_summaries;

  TensorTypeSummary(TypeSummary &element_type, uint64_t num_dimensions);
  // TensorTypeSummary(TypeSummary *element_type,
  //                   std::vector<uint64_t> &length_of_dimensions);

  friend class TypeAnalysis;
};

struct ReferenceTypeSummary : public TypeSummary {
public:
  static ReferenceTypeSummary &get(TypeSummary &referenced_type);

  TypeSummary &getReferencedType();

  std::string toString(std::string indent = "") const override;

protected:
  TypeSummary &referenced_type;

  static map<TypeSummary *, ReferenceTypeSummary *> reference_type_summaries;

  ReferenceTypeSummary(TypeSummary &referenced_type);

  friend class TypeAnalysis;
};

struct IntegerTypeSummary : public TypeSummary {
public:
  static IntegerTypeSummary &get(unsigned bitwidth, bool is_signed);

  unsigned getBitWidth();
  bool isSigned();

  std::string toString(std::string indent = "") const override;

protected:
  unsigned bitwidth;
  bool is_signed;

  static map<unsigned, /* bitwidth */
             map<bool, /* is signed? */
                 IntegerTypeSummary *>>
      integer_type_summaries;

  IntegerTypeSummary(unsigned bitwidth, bool is_signed);

  friend class TypeAnalysis;
};

struct FloatTypeSummary : public TypeSummary {
public:
  static FloatTypeSummary &get();

  std::string toString(std::string indent = "") const override;

protected:
  FloatTypeSummary();

  friend class TypeAnalysis;
};

struct DoubleTypeSummary : public TypeSummary {
public:
  static DoubleTypeSummary &get();

  std::string toString(std::string indent = "") const override;

protected:
  DoubleTypeSummary();

  friend class TypeAnalysis;
};

} // namespace llvm::memoir

#endif // COMMON_TYPES_H
