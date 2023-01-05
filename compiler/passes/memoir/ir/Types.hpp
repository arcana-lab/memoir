#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H
#pragma once

#include <cstdio>

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

#include "memoir/support/InternalDatatypes.hpp"

#include "memoir/utility/FunctionNames.hpp"

#include "memoir/ir/Instructions.hpp"

namespace llvm::memoir {

enum class TypeCode {
  INTEGER,
  FLOAT,
  DOUBLE,
  POINTER,
  REFERENCE,
  STRUCT,
  FIELD_ARRAY,
  STATIC_TENSOR,
  TENSOR,
  ASSOC_ARRAY,
  SEQUENCE,
};

struct IntegerType;
struct FloatType;
struct DoubleType;
struct PointerType;
struct ReferenceType;
struct StructType;
struct FieldArrayType;
struct StaticTensorType;
struct TensorType;
struct AssocArrayType;
struct SequenceType;

struct DefineStructTypeInst;

struct Type {
public:
  static IntegerType &get_u64_type();
  static IntegerType &get_u32_type();
  static IntegerType &get_u16_type();
  static IntegerType &get_u8_type();
  static IntegerType &get_i64_type();
  static IntegerType &get_i32_type();
  static IntegerType &get_i16_type();
  static IntegerType &get_i8_type();
  static IntegerType &get_i1_type();
  static FloatType &get_f32_type();
  static DoubleType &get_f64_type();
  static PointerType &get_ptr_type();
  static ReferenceType &get_ref_type(Type &referenced_type);
  static StructType &define_struct_type(DefineStructTypeInst &definition,
                                        std::string name,
                                        vector<Type *> field_types);
  static StructType &get_struct_type(std::string name);
  static FieldArrayType &get_field_array_type(StructType &type,
                                              unsigned field_index);
  static StaticTensorType &get_static_tensor_type(
      Type &element_type,
      vector<size_t> dimension_lengths);
  static TensorType &get_tensor_type(Type &element_type,
                                     unsigned num_dimensions);
  static AssocArrayType &get_assoc_array_type(Type &key_type, Type &value_type);
  static SequenceType &get_sequence_type(Type &element_type);

  static bool is_primitive_type(Type &type);
  static bool is_reference_type(Type &type);
  static bool is_struct_type(Type &type);
  static bool is_collection_type(Type &type);

  TypeCode getCode() const;

  // TODO: implement conversion to LLVM type
  // virtual llvm::Type *getLLVMType() const;

  virtual std::string toString(std::string indent = "") const = 0;

  friend std::ostream &operator<<(std::ostream &os, const Type &T);
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const Type &T);
  friend bool operator<(const Type &L, const Type &R);

protected:
  TypeCode code;

  Type(TypeCode code);

  friend class TypeAnalysis;
};

struct IntegerType : public Type {
public:
  template <unsigned BW, bool S>
  static IntegerType &get();

  unsigned getBitWidth() const;
  bool isSigned() const;

  std::string toString(std::string indent = "") const override;

protected:
  unsigned bitwidth;
  bool is_signed;

  IntegerType(unsigned bitwidth, bool is_signed);

  friend class TypeAnalysis;
};

struct FloatType : public Type {
public:
  static FloatType &get();

  std::string toString(std::string indent = "") const override;

protected:
  FloatType();

  friend class TypeAnalysis;
};

struct DoubleType : public Type {
public:
  static DoubleType &get();

  std::string toString(std::string indent = "") const override;

protected:
  DoubleType();

  friend class TypeAnalysis;
};

struct PointerType : public Type {
public:
  static PointerType &get();

  std::string toString(std::string indent = "") const override;

protected:
  PointerType();

  friend class TypeAnalysis;
};

struct ReferenceType : public Type {
public:
  static ReferenceType &get(Type &referenced_type);

  Type &getReferencedType() const;

  std::string toString(std::string indent = "") const override;

protected:
  Type &referenced_type;

  static map<Type *, ReferenceType *> reference_types;

  ReferenceType(Type &referenced_type);

  friend class TypeAnalysis;
};

struct StructType : public Type {
public:
  static StructType &define(DefineStructTypeInst &definition,
                            std::string name,
                            vector<Type *> field_types);
  static StructType &get(std::string name);

  DefineStructTypeInst &getDefinition() const;
  std::string getName() const;
  unsigned getNumFields() const;
  Type &getFieldType(unsigned field_index) const;

  std::string toString(std::string indent = "") const override;

protected:
  DefineStructTypeInst &definition;
  std::string name;
  vector<Type *> field_types;

  static map<std::string, StructType *> defined_types;

  StructType(DefineStructTypeInst &definition,
             std::string name,
             vector<Type *> field_types);

  friend class TypeAnalysis;
};

struct CollectionType : public Type {
public:
  virtual Type &getElementType() const = 0;

protected:
  CollectionType(TypeCode code);

  friend class TypeAnalysis;
};

struct FieldArrayType : public CollectionType {
public:
  static FieldArrayType &get(StructType &struct_type, unsigned field_index);

  Type &getElementType() const override;

  StructType &getStructType() const;
  unsigned getFieldIndex() const;

  std::string toString(std::string indent = "") const override;

protected:
  StructType &struct_type;
  unsigned field_index;

  static map<StructType *, map<unsigned, FieldArrayType *>>
      struct_to_field_array;

  FieldArrayType(StructType &struct_type, unsigned field_index);
};

struct StaticTensorType : public CollectionType {
public:
  Type &getElementType() const override;
  unsigned getNumberOfDimensions() const;
  size_t getLengthOfDimension(unsigned dimension_index) const;

  std::string toString(std::string indent = "") const override;

protected:
  Type &element_type;
  unsigned number_of_dimensions;
  vector<size_t> length_of_dimensions;

  StaticTensorType(Type &element_type,
                   unsigned number_of_dimensions,
                   vector<size_t> length_of_dimensions);

  friend class TypeAnalysis;
};

struct TensorType : public CollectionType {
public:
  Type &getElementType() const override;
  unsigned getNumberOfDimensions() const;

  std::string toString(std::string indent = "") const override;

protected:
  Type &element_type;
  unsigned number_of_dimensions;

  TensorType(Type &element_type, unsigned number_of_dimensions);

  friend class TypeAnalysis;
};

struct AssocArrayType : public CollectionType {
public:
  static AssocArrayType &get(Type &key_type, Type &value_type);

  Type &getKeyType() const;
  Type &getValueType() const;
  Type &getElementType() const override;

  std::string toString(std::string indent = "") const override;

protected:
  Type &key_type;
  Type &value_type;

  static map<Type *, map<Type *, AssocArrayType *>> assoc_array_type_summaries;

  AssocArrayType(Type &key_type, Type &value_type);

  friend class TypeAnalysis;
};

struct SequenceType : public CollectionType {
public:
  static SequenceType &get(Type &element_type);

  Type &getElementType() const override;

  std::string toString(std::string indent = "") const override;

protected:
  Type &element_type;

  static map<Type *, SequenceType *> assoc_array_type_summaries;

  SequenceType(Type &element_type);

  friend class TypeAnalysis;
};

} // namespace llvm::memoir

#endif
