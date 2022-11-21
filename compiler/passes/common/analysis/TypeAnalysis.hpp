#ifndef COMMON_TYPEANALYSIS_H
#define COMMON_TYPEANALYSIS_H
#pragma once

#include <iostream>
#include <string>

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"

#include "common/analysis/CollectionAnalysis.hpp"
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
class FieldArraySummary;

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

  TypeSummary &getStaticTensorTypeSummary(llvm::CallInst &call_inst);

  TypeSummary &getAssocArrayTypeSummary(llvm::CallInst &call_inst);

  TypeSummary &getSequenceTypeSummary(llvm::CallInst &call_inst);

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
  StaticTensorTy,
  AssocArrayTy,
  SequenceTy,
  IntegerTy,
  FloatTy,
  DoubleTy,
  ReferenceTy,
};

struct TypeSummary {
public:
  TypeCode getCode() const;

  // TODO: change this to be an operator== override
  bool equals(TypeSummary *other) const;
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

class FieldArraySummary;

struct StructTypeSummary : public TypeSummary {
public:
  static StructTypeSummary &get(std::string name);
  static StructTypeSummary &get(std::string name,
                                std::vector<TypeSummary *> &field_types,
                                llvm::CallInst &call_inst);
  static StructTypeSummary &get(std::string name,
                                std::vector<TypeSummary *> &field_types,
                                TypeSummary &container);

  std::string getName() const;
  uint64_t getNumFields() const;
  bool fieldIsANestedStruct(uint64_t field_index) const;
  TypeSummary &getField(uint64_t field_index) const;
  bool isFieldArray(uint64_t field_index) const;
  FieldArraySummary &getFieldArray(uint64_t field_index) const;

  /*
   * If this is a base Struct Type
   */
  bool isBase() const;
  llvm::CallInst &getCallInst() const;

  /*
   * If this is a nested Struct Type
   */
  bool isNested() const;
  StructTypeSummary &getContainer() const;
  uint64_t getContainerFieldIndex() const;

  std::string toString(std::string indent = "") const override;

protected:
  llvm::CallInst *call_inst;
  std::string name;
  vector<TypeSummary *> field_types;
  vector<FieldArraySummary *> field_arrays;

  StructTypeSummary *container;
  uint64_t field_index_of_container;

  static map<std::string, StructTypeSummary *> defined_type_summaries;

  StructTypeSummary(std::string name,
                    vector<TypeSummary *> &field_types,
                    vector<FieldArraySummary *> &field_arrays);
  StructTypeSummary(std::string name,
                    vector<TypeSummary *> &field_types,
                    vector<FieldArraySummary *> &field_arrays,
                    llvm::CallInst &call_inst);
  StructTypeSummary(std::string name,
                    vector<TypeSummary *> &field_types,
                    vector<FieldArraySummary *> &field_arrays,
                    StructTypeSummary &container,
                    uint64_t field_index_of_container);

  friend class TypeAnalysis;
};

struct TensorTypeSummary : public TypeSummary {
public:
  static TensorTypeSummary &get(TypeSummary &element_type,
                                uint64_t num_dimensions);

  TypeSummary &getElementType() const;
  uint64_t getNumDimensions() const;

  std::string toString(std::string indent = "") const override;

protected:
  TypeSummary &element_type;
  uint64_t num_dimensions;

  static map<TypeSummary *, map<uint64_t, TensorTypeSummary *>>
      tensor_type_summaries;

  TensorTypeSummary(TypeSummary &element_type, uint64_t num_dimensions);

  friend class TypeAnalysis;
};

struct StaticTensorTypeSummary : public TypeSummary {
public:
  llvm::CallInst &getCall() const;
  TypeSummary &getElementType() const;
  uint64_t getNumDimensions() const;
  uint64_t getLengthOfDimension(uint64_t dimension_index) const;

  std::string toString(std::string indent = "") const override;

protected:
  TypeSummary &element_type;
  llvm::CallInst &call_inst;
  vector<uint64_t> length_of_dimensions;

  StaticTensorTypeSummary(TypeSummary &element_type,
                          std::vector<uint64_t> &length_of_dimensions);

  friend class TypeAnalysis;
};

struct AssocArrayTypeSummary : public TypeSummary {
public:
  static AssocArrayTypeSummary &get(TypeSummary &key_type,
                                    TypeSummary &value_type);

  TypeSummary &getKeyType() const;
  TypeSummary &getValueType() const;

  std::string toString(std::string indent = "") const override;

protected:
  TypeSummary &key_type;
  TypeSummary &value_type;

  static map<TypeSummary *, map<TypeSummary *, AssocArrayTypeSummary *>>
      assoc_array_type_summaries;

  AssocArrayTypeSummary(TypeSummary &key_type, TypeSummary &value_type);

  friend class TypeAnalysis;
};

struct SequenceTypeSummary : public TypeSummary {
public:
  static SequenceTypeSummary &get(TypeSummary &key_type,
                                  TypeSummary &value_type);

  TypeSummary &getElementType() const;

  std::string toString(std::string indent = "") const override;

protected:
  TypeSummary &element_type;

  static map<TypeSummary *, SequenceTypeSummary *> assoc_array_type_summaries;

  SequenceTypeSummary(TypeSummary &element_type);

  friend class TypeAnalysis;
};

struct ReferenceTypeSummary : public TypeSummary {
public:
  static ReferenceTypeSummary &get(TypeSummary &referenced_type);

  TypeSummary &getReferencedType() const;

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

  unsigned getBitWidth() const;
  bool isSigned() const;

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
