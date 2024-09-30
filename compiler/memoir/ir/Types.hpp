#ifndef MEMOIR_IR_TYPES_H
#define MEMOIR_IR_TYPES_H
#pragma once

#include <cstdio>

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

#include "memoir/support/Assert.hpp"
#include "memoir/support/InternalDatatypes.hpp"

#include "memoir/utility/FunctionNames.hpp"

#include "memoir/ir/Instructions.hpp"

namespace llvm::memoir {

enum class TypeCode {
  INTEGER,
  FLOAT,
  DOUBLE,
  POINTER,
  VOID,
  REFERENCE,
  STRUCT,
  FIELD_ARRAY,
  STATIC_TENSOR,
  TENSOR,
  ASSOC_ARRAY,
  SEQUENCE,
  OTHER, // A special code for extensibility
};

struct IntegerType;
struct FloatType;
struct DoubleType;
struct PointerType;
struct VoidType;
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
  static IntegerType &get_u2_type();
  static IntegerType &get_i64_type();
  static IntegerType &get_i32_type();
  static IntegerType &get_i16_type();
  static IntegerType &get_i8_type();
  static IntegerType &get_i2_type();
  static IntegerType &get_bool_type();
  static IntegerType &get_size_type(const llvm::DataLayout &DL);
  static FloatType &get_f32_type();
  static DoubleType &get_f64_type();
  static PointerType &get_ptr_type();
  static VoidType &get_void_type();
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

  static Type &from_code(std::string code);

  static bool is_primitive_type(Type &type);
  static bool is_reference_type(Type &type);
  static bool is_struct_type(Type &type);
  static bool is_collection_type(Type &type);
  static bool value_is_collection_type(llvm::Value &value);
  static bool value_is_struct_type(llvm::Value &value);

  TypeCode getCode() const;

  // TODO: implement conversion to LLVM type
  virtual llvm::Type *get_llvm_type(llvm::LLVMContext &C) const;

  virtual std::string toString(std::string indent = "") const = 0;
  virtual opt<std::string> get_code() const;

  friend std::ostream &operator<<(std::ostream &os, const Type &T);
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const Type &T);
  friend bool operator<(const Type &L, const Type &R);

  virtual ~Type() = default;

protected:
  TypeCode code;

  Type(TypeCode code);
};

struct IntegerType : public Type {
public:
  // Creation.
  template <unsigned BW, bool S>
  static IntegerType &get();

  // Access.
  unsigned getBitWidth() const;
  bool isSigned() const;

  // RTTI.
  static bool classof(const Type *T) {
    return (T->getCode() == TypeCode::INTEGER);
  }

  // Debug.
  std::string toString(std::string indent = "") const override;
  opt<std::string> get_code() const override;

  llvm::Type *get_llvm_type(llvm::LLVMContext &C) const override;

protected:
  unsigned bitwidth;
  bool is_signed;

  IntegerType(unsigned bitwidth, bool is_signed);

  friend struct Type;
};

struct FloatType : public Type {
public:
  static FloatType &get();

  static bool classof(const Type *T) {
    return (T->getCode() == TypeCode::FLOAT);
  }

  std::string toString(std::string indent = "") const override;
  opt<std::string> get_code() const override;

  llvm::Type *get_llvm_type(llvm::LLVMContext &C) const override;

protected:
  FloatType();
};

struct DoubleType : public Type {
public:
  static DoubleType &get();

  static bool classof(const Type *T) {
    return (T->getCode() == TypeCode::DOUBLE);
  }

  std::string toString(std::string indent = "") const override;
  opt<std::string> get_code() const override;

  llvm::Type *get_llvm_type(llvm::LLVMContext &C) const override;

protected:
  DoubleType();
};

struct PointerType : public Type {
public:
  static PointerType &get();

  static bool classof(const Type *T) {
    return (T->getCode() == TypeCode::POINTER);
  }

  std::string toString(std::string indent = "") const override;
  opt<std::string> get_code() const override;

  llvm::Type *get_llvm_type(llvm::LLVMContext &C) const override;

protected:
  PointerType();
};

struct VoidType : public Type {
public:
  static VoidType &get();

  static bool classof(const Type *T) {
    return (T->getCode() == TypeCode::VOID);
  }

  std::string toString(std::string indent = "") const override;
  opt<std::string> get_code() const override;

  llvm::Type *get_llvm_type(llvm::LLVMContext &C) const override;

protected:
  VoidType();
};

struct ReferenceType : public Type {
public:
  static ReferenceType &get(Type &referenced_type);

  Type &getReferencedType() const;

  static bool classof(const Type *T) {
    return (T->getCode() == TypeCode::REFERENCE);
  }

  std::string toString(std::string indent = "") const override;
  opt<std::string> get_code() const override;

  llvm::Type *get_llvm_type(llvm::LLVMContext &C) const override;

protected:
  Type &referenced_type;

  static map<Type *, ReferenceType *> *reference_types;

  ReferenceType(Type &referenced_type);
};

struct StructType : public Type {
public:
  // Creation.
  static StructType &define(DefineStructTypeInst &definition,
                            std::string name,
                            vector<Type *> field_types);
  static StructType &get(std::string name);

  // Access.
  DefineStructTypeInst &getDefinition() const;
  std::string getName() const;
  unsigned getNumFields() const;
  Type &getFieldType(unsigned field_index) const;

  llvm::Type *get_llvm_type(llvm::LLVMContext &C) const override;

  // RTTI.
  static bool classof(const Type *T) {
    return (T->getCode() == TypeCode::STRUCT);
  }

  // Debug.
  std::string toString(std::string indent = "") const override;
  opt<std::string> get_code() const override;

protected:
  DefineStructTypeInst &definition;
  std::string name;
  vector<Type *> field_types;

  static map<std::string, StructType *> *defined_types;

  StructType(DefineStructTypeInst &definition,
             std::string name,
             vector<Type *> field_types);
};

struct CollectionType : public Type {
public:
  virtual Type &getElementType() const = 0;

  static bool classof(const Type *T) {
    switch (T->getCode()) {
      default:
        return false;
      case TypeCode::FIELD_ARRAY:
      case TypeCode::STATIC_TENSOR:
      case TypeCode::TENSOR:
      case TypeCode::SEQUENCE:
      case TypeCode::ASSOC_ARRAY:
        return true;
    };
  }

  opt<std::string> get_code() const override;

  llvm::Type *get_llvm_type(llvm::LLVMContext &C) const override;

protected:
  CollectionType(TypeCode code);
};

struct FieldArrayType : public CollectionType {
public:
  static FieldArrayType &get(StructType &struct_type, unsigned field_index);

  Type &getElementType() const override;

  StructType &getStructType() const;
  unsigned getFieldIndex() const;

  static bool classof(const Type *T) {
    return (T->getCode() == TypeCode::FIELD_ARRAY);
  }

  std::string toString(std::string indent = "") const override;

protected:
  StructType &struct_type;
  unsigned field_index;

  static map<StructType *, map<unsigned, FieldArrayType *>>
      *struct_to_field_array;

  FieldArrayType(StructType &struct_type, unsigned field_index);
};

struct StaticTensorType : public CollectionType {
public:
  static StaticTensorType &get(Type &element_type,
                               vector<size_t> length_of_dimensions);

  Type &getElementType() const override;
  unsigned getNumberOfDimensions() const;
  size_t getLengthOfDimension(unsigned dimension_index) const;

  static bool classof(const Type *T) {
    return (T->getCode() == TypeCode::STATIC_TENSOR);
  }

  std::string toString(std::string indent = "") const override;

protected:
  Type &element_type;
  unsigned number_of_dimensions;
  vector<size_t> length_of_dimensions;

  StaticTensorType(Type &element_type,
                   unsigned number_of_dimensions,
                   vector<size_t> length_of_dimensions);

  static ordered_multimap<Type *, StaticTensorType *> *static_tensor_types;
};

struct TensorType : public CollectionType {
public:
  static TensorType &get(Type &element_type, unsigned num_dimensions);

  Type &getElementType() const override;
  unsigned getNumberOfDimensions() const;

  static bool classof(const Type *T) {
    return (T->getCode() == TypeCode::TENSOR);
  }

  std::string toString(std::string indent = "") const override;

protected:
  Type &element_type;
  unsigned number_of_dimensions;

  TensorType(Type &element_type, unsigned number_of_dimensions);

  static map<Type *, map<unsigned, TensorType *>> *tensor_types;
};

struct AssocArrayType : public CollectionType {
public:
  static AssocArrayType &get(Type &key_type, Type &value_type);

  Type &getKeyType() const;
  Type &getValueType() const;
  Type &getElementType() const override;

  static bool classof(const Type *T) {
    return (T->getCode() == TypeCode::ASSOC_ARRAY);
  }

  std::string toString(std::string indent = "") const override;

protected:
  Type &key_type;
  Type &value_type;

  static map<Type *, map<Type *, AssocArrayType *>> *assoc_array_types;

  AssocArrayType(Type &key_type, Type &value_type);
};
using AssocType = struct AssocArrayType;

struct SequenceType : public CollectionType {
public:
  static SequenceType &get(Type &element_type);

  Type &getElementType() const override;

  static bool classof(const Type *T) {
    return (T->getCode() == TypeCode::SEQUENCE);
  }

  std::string toString(std::string indent = "") const override;

protected:
  Type &element_type;

  static map<Type *, SequenceType *> *sequence_types;

  SequenceType(Type &element_type);
};

/**
 * Query the MEMOIR type of an LLVM Value.
 *
 * @param V an LLVM Value
 * @returns the MEMOIR type of V, or NULL if it is has none.
 */
Type *type_of(llvm::Value &V);

struct MemOIRInst;

/**
 * Query the MEMOIR type of an MEMOIR instruction.
 *
 * @param I a MEMOIR instruction
 * @returns the MEMOIR type of V, or NULL if it is has none.
 */
Type *type_of(MemOIRInst &I);

} // namespace llvm::memoir

#endif
