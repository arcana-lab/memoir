#ifndef MEMOIR_IR_TYPES_H
#define MEMOIR_IR_TYPES_H
#pragma once

#include <cstdio>

#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

#include "memoir/ir/Instructions.hpp"
#include "memoir/support/Assert.hpp"
#include "memoir/support/Casting.hpp"
#include "memoir/support/DataTypes.hpp"
#include "memoir/utility/FunctionNames.hpp"

namespace memoir {

enum class TypeKind {
  INTEGER,
  FLOAT,
  DOUBLE,
  POINTER,
  VOID,
  REFERENCE,
  TUPLE,
  ARRAY,
  ASSOC_ARRAY,
  SEQUENCE,
  VARIABLE,
  OTHER, // A special code for extensibility
};

struct IntegerType;
struct FloatType;
struct DoubleType;
struct PointerType;
struct VoidType;
struct ReferenceType;
struct TupleType;
struct ArrayType;
struct AssocArrayType;
struct SequenceType;

struct DefineTupleTypeInst;

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
  static Type &define_type(std::string name, Type &type);
  static Type &lookup_type(std::string name);
  static TupleType &get_tuple_type(llvm::ArrayRef<Type *> fields);
  static ArrayType &get_array_type(Type &element_type, size_t length);

  static Type &from_code(std::string code);

  static bool is_primitive_type(Type &type);
  static bool is_reference_type(Type &type);
  static bool is_struct_type(Type &type);
  static bool is_collection_type(Type &type);
  static bool is_unsized(Type &type);
  static bool value_is_object(llvm::Value &value);
  static bool value_is_collection_type(llvm::Value &value);
  static bool value_is_struct_type(llvm::Value &value);

  TypeKind getKind() const;

  // TODO: implement conversion to LLVM type
  virtual llvm::Type *get_llvm_type(llvm::LLVMContext &C) const;

  virtual std::string toString(std::string indent = "") const = 0;
  virtual Option<std::string> get_code() const;

  friend std::ostream &operator<<(std::ostream &os, const Type &T);
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os, const Type &T);

  virtual bool operator==(const Type &other) const;
  virtual bool operator<=(const Type &other) const;

  virtual ~Type() = default;

protected:
  TypeKind code;

  Type(TypeKind code);
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
    return (T->getKind() == TypeKind::INTEGER);
  }

  // Debug.
  std::string toString(std::string indent = "") const override;
  Option<std::string> get_code() const override;

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
    return (T->getKind() == TypeKind::FLOAT);
  }

  std::string toString(std::string indent = "") const override;
  Option<std::string> get_code() const override;

  llvm::Type *get_llvm_type(llvm::LLVMContext &C) const override;

protected:
  FloatType();
};

struct DoubleType : public Type {
public:
  static DoubleType &get();

  static bool classof(const Type *T) {
    return (T->getKind() == TypeKind::DOUBLE);
  }

  std::string toString(std::string indent = "") const override;
  Option<std::string> get_code() const override;

  llvm::Type *get_llvm_type(llvm::LLVMContext &C) const override;

protected:
  DoubleType();
};

struct PointerType : public Type {
public:
  static PointerType &get();

  static bool classof(const Type *T) {
    return (T->getKind() == TypeKind::POINTER);
  }

  std::string toString(std::string indent = "") const override;
  Option<std::string> get_code() const override;

  llvm::Type *get_llvm_type(llvm::LLVMContext &C) const override;

protected:
  PointerType();
};

struct VoidType : public Type {
public:
  static VoidType &get();

  static bool classof(const Type *T) {
    return (T->getKind() == TypeKind::VOID);
  }

  std::string toString(std::string indent = "") const override;
  Option<std::string> get_code() const override;

  llvm::Type *get_llvm_type(llvm::LLVMContext &C) const override;

protected:
  VoidType();
};

struct ReferenceType : public Type {
public:
  static ReferenceType &get(Type &referenced_type);

  Type &getReferencedType() const;

  static bool classof(const Type *T) {
    return (T->getKind() == TypeKind::REFERENCE);
  }

  std::string toString(std::string indent = "") const override;
  Option<std::string> get_code() const override;

  llvm::Type *get_llvm_type(llvm::LLVMContext &C) const override;

protected:
  Type &referenced_type;

  static Map<Type *, ReferenceType *> *reference_types;

  ReferenceType(Type &referenced_type);
};

struct ObjectType : public Type {
public:
  static bool classof(const Type *T) {
    switch (T->getKind()) {
      default:
        return false;
      case TypeKind::TUPLE:
      case TypeKind::ARRAY:
      case TypeKind::SEQUENCE:
      case TypeKind::ASSOC_ARRAY:
        return true;
    };
  }

protected:
  ObjectType(TypeKind code);
};

struct TupleType : public ObjectType {
public:
  // Creation.
  static TupleType &get(llvm::ArrayRef<Type *> field_types);

  // Access.
  unsigned getNumFields() const;
  Type &getFieldType(unsigned field) const;
  llvm::ArrayRef<Type *> fields() const;

  llvm::Type *get_llvm_type(llvm::LLVMContext &C) const override;

  // RTTI.
  static bool classof(const Type *T) {
    return (T->getKind() == TypeKind::TUPLE);
  }

  // Debug.
  std::string toString(std::string indent = "") const override;
  Option<std::string> get_code() const override;

protected:
  Vector<Type *> field_types;

  static OrderedMultiMap<unsigned, TupleType *> *tuple_types;

  TupleType(llvm::ArrayRef<Type *> fields);
};

struct CollectionType : public ObjectType {
public:
  virtual Type &getElementType() const = 0;

  static bool classof(const Type *T) {
    switch (T->getKind()) {
      default:
        return false;
      case TypeKind::ARRAY:
      case TypeKind::SEQUENCE:
      case TypeKind::ASSOC_ARRAY:
        return true;
    };
  }

  Option<std::string> get_code() const override;

  virtual Option<std::string> get_selection() const;
  virtual CollectionType &set_selection(Option<std::string> selection);

  llvm::Type *get_llvm_type(llvm::LLVMContext &C) const override;

protected:
  CollectionType(TypeKind code);
};

struct ArrayType : public CollectionType {
public:
  static ArrayType &get(Type &element_type, size_t length);

  Type &getElementType() const override;
  size_t getLength() const;

  static bool classof(const Type *T) {
    return (T->getKind() == TypeKind::ARRAY);
  }

  std::string toString(std::string indent = "") const override;

protected:
  Type &element_type;
  size_t length;

  ArrayType(Type &element_type, size_t length);

  static OrderedMultiMap<Type *, ArrayType *> *array_types;
};

struct AssocArrayType : public CollectionType {
public:
  static AssocArrayType &get(Type &key_type,
                             Type &value_type,
                             std::optional<std::string> selection = {});

  Type &getKeyType() const;
  Type &getValueType() const;
  Type &getElementType() const override;

  Option<std::string> get_selection() const override;
  CollectionType &set_selection(Option<std::string> selection) override;

  static bool classof(const Type *T) {
    return (T->getKind() == TypeKind::ASSOC_ARRAY);
  }

  std::string toString(std::string indent = "") const override;

  Option<std::string> get_code() const override;

  bool operator<=(const Type &other) const override;

protected:
  Type &key_type;
  Type &value_type;
  Option<std::string> selection;

  typedef OrderedMap<
      Type *,
      OrderedMap<Type *,
                 OrderedMap<std::optional<std::string>, AssocArrayType *>>>
      Types;
  static Types *assoc_array_types;

  AssocArrayType(Type &key_type,
                 Type &value_type,
                 Option<std::string> selection = {});
};
using AssocType = struct AssocArrayType;

struct SequenceType : public CollectionType {
public:
  static SequenceType &get(Type &element_type,
                           std::optional<std::string> selection = {});

  Type &getElementType() const override;

  static bool classof(const Type *T) {
    return (T->getKind() == TypeKind::SEQUENCE);
  }

  std::string toString(std::string indent = "") const override;

  Option<std::string> get_code() const override;

  Option<std::string> get_selection() const override;
  CollectionType &set_selection(Option<std::string> selection) override;

  bool operator<=(const Type &other) const override;

protected:
  Type &element_type;
  Option<std::string> selection;

  typedef OrderedMap<Type *,
                     OrderedMap<std::optional<std::string>, SequenceType *>>
      Types;
  static Types *sequence_types;

  SequenceType(Type &element_type, Option<std::string> selection = {});
};

/**
 * A type variable used for unification.
 */
struct TypeVariable : public Type {
public:
  using TypeID = uint64_t;

  static TypeVariable &get() {
    static TypeID id = 0;
    auto *var = new TypeVariable(id++);
    return *var;
  }

  // Equality.
  bool operator==(Type &T) const {
    if (auto *tvar = dyn_cast<TypeVariable>(&T)) {
      return tvar->id == this->id;
    }
    return false;
  }

  // This class will only be used in the context of the base types and
  // itself, so it is the only one that follows "other".
  static bool classof(const Type *t) {
    return (t->getKind() == TypeKind::VARIABLE);
  }

  std::string toString(std::string indent = "") const override {
    return "typevar(" + std::to_string(this->id) + ")";
  }

protected:
  TypeVariable(TypeID id) : Type(TypeKind::VARIABLE), id(id) {}

  TypeID id;
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

} // namespace memoir

#endif
