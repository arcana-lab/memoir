#ifndef COMMON_INSTRUCTIONS_H
#define COMMON_INSTRUCTIONS_H
#pragma once

#include <cstddef>

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

#include "memoir/utility/FunctionNames.hpp"

#include "memoir/support/Casting.hpp"

#include "memoir/ir/Collections.hpp"
#include "memoir/ir/Function.hpp"
#include "memoir/ir/Structs.hpp"
#include "memoir/ir/Types.hpp"

/*
 * MemOIR Instructions and a wrapper of an LLVM Instruction.
 *
 * Author(s): Tommy McMichen
 * Created: December 13, 2022
 */

namespace llvm::memoir {

struct MemOIRFunction;
struct Collection;
struct CollectionType;
struct FieldArray;
struct Struct;

/*
 * Abstract MemOIR Instruction
 */
struct MemOIRInst {
public:
  static MemOIRInst *get(llvm::Instruction &I);

  MemOIRFunction &getFunction() const;
  llvm::CallInst &getCallInst() const;
  MemOIR_Func getKind() const;

  friend std::ostream &operator<<(std::ostream &os, const MemOIRInst &I);

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                       const MemOIRInst &I);
  virtual std::string toString(std::string indent = "") const = 0;

protected:
  llvm::CallInst &call_inst;

  MemOIRInst(llvm::CallInst &call_inst);

  static map<llvm::Instruction *, MemOIRInst *> llvm_to_memoir;
};

/*
 * Types
 */
struct TypeInst : public MemOIRInst {
public:
  virtual Type &getType() const = 0;

  static bool classof(const MemOIRInst *I) {
    return
#define HANDLE_TYPE_INST(ENUM, FUNC, CLASS)                                    \
  (I->getKind() == MemOIR_Func::ENUM) ||
#include "memoir/ir/Instructions.def"
        false;
  };

protected:
  TypeInst(llvm::CallInst &call_inst);

  friend class MemOIRInst;
};

struct UInt64TypeInst : public TypeInst {
public:
  Type &getType() const override;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::UINT64_TYPE);
  };

  std::string toString(std::string indent = "") const override;

protected:
  UInt64TypeInst(llvm::CallInst &call_inst);

  friend class MemOIRInst;
};

struct UInt32TypeInst : public TypeInst {
public:
  Type &getType() const override;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::UINT32_TYPE);
  };

  std::string toString(std::string indent = "") const override;

protected:
  UInt32TypeInst(llvm::CallInst &call_inst);

  friend class MemOIRInst;
};

struct UInt16TypeInst : public TypeInst {
public:
  Type &getType() const override;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::UINT16_TYPE);
  };

  std::string toString(std::string indent = "") const override;

protected:
  UInt16TypeInst(llvm::CallInst &call_inst);

  friend class MemOIRInst;
};

struct UInt8TypeInst : public TypeInst {
public:
  Type &getType() const override;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::UINT8_TYPE);
  };

  std::string toString(std::string indent = "") const override;

protected:
  UInt8TypeInst(llvm::CallInst &call_inst);

  friend class MemOIRInst;
};

struct Int64TypeInst : public TypeInst {
public:
  Type &getType() const override;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::INT64_TYPE);
  };

  std::string toString(std::string indent = "") const override;

protected:
  Int64TypeInst(llvm::CallInst &call_inst);

  friend class MemOIRInst;
};

struct Int32TypeInst : public TypeInst {
public:
  Type &getType() const override;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::INT32_TYPE);
  };

  std::string toString(std::string indent = "") const override;

protected:
  Int32TypeInst(llvm::CallInst &call_inst);

  friend class MemOIRInst;
};

struct Int16TypeInst : public TypeInst {
public:
  Type &getType() const override;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::INT16_TYPE);
  };

  std::string toString(std::string indent = "") const override;

protected:
  Int16TypeInst(llvm::CallInst &call_inst);

  friend class MemOIRInst;
};

struct Int8TypeInst : public TypeInst {
public:
  Type &getType() const override;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::INT8_TYPE);
  };

  std::string toString(std::string indent = "") const override;

protected:
  Int8TypeInst(llvm::CallInst &call_inst);

  friend class MemOIRInst;
};

struct BoolTypeInst : public TypeInst {
public:
  Type &getType() const override;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::BOOL_TYPE);
  };

  std::string toString(std::string indent = "") const override;

protected:
  BoolTypeInst(llvm::CallInst &call_inst);

  friend class MemOIRInst;
};

struct FloatTypeInst : public TypeInst {
public:
  Type &getType() const override;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::FLOAT_TYPE);
  };

  std::string toString(std::string indent = "") const override;

protected:
  FloatTypeInst(llvm::CallInst &call_inst);

  friend class MemOIRInst;
};

struct DoubleTypeInst : public TypeInst {
public:
  Type &getType() const override;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::DOUBLE_TYPE);
  };

  std::string toString(std::string indent = "") const override;

protected:
  DoubleTypeInst(llvm::CallInst &call_inst);

  friend class MemOIRInst;
};

struct PointerTypeInst : public TypeInst {
public:
  Type &getType() const override;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::POINTER_TYPE);
  };

  std::string toString(std::string indent = "") const override;

protected:
  PointerTypeInst(llvm::CallInst &call_inst);

  friend class MemOIRInst;
};

struct ReferenceTypeInst : public TypeInst {
public:
  Type &getType() const override;
  Type &getReferencedType() const;
  llvm::Value &getReferencedTypeOperand() const;
  llvm::Use &getReferencedTypeAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::REFERENCE_TYPE);
  };

  std::string toString(std::string indent = "") const override;

protected:
  ReferenceTypeInst(llvm::CallInst &call_inst);

  friend class MemOIRInst;
};

struct DefineStructTypeInst : public TypeInst {
public:
  Type &getType() const override;
  std::string getName() const;
  llvm::Value &getNameOperand() const;
  llvm::Use &getNameOperandAsUse() const;
  unsigned getNumberOfFields() const;
  llvm::Value &getNumberOfFieldsOperand() const;
  llvm::Use &getNumberOfFieldsOperandAsUse() const;
  Type &getFieldType(unsigned field_index) const;
  llvm::Value &getFieldTypeOperand(unsigned field_index) const;
  llvm::Use &getFieldTypeOperandAsUse(unsigned field_index) const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::DEFINE_STRUCT_TYPE);
  };

  std::string toString(std::string indent = "") const override;

protected:
  DefineStructTypeInst(llvm::CallInst &call_inst);

  friend class MemOIRInst;
};

struct StructTypeInst : public TypeInst {
public:
  Type &getType() const override;
  std::string getName() const;
  llvm::Value &getNameOperand() const;
  llvm::Use &getNameOperandAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::STRUCT_TYPE);
  };

  std::string toString(std::string indent = "") const override;

protected:
  StructTypeInst(llvm::CallInst &call_inst);

  friend class MemOIRInst;
};

struct StaticTensorTypeInst : public TypeInst {
public:
  Type &getType() const override;
  Type &getElementType() const;
  llvm::Value &getElementTypeOperand() const;
  llvm::Use &getElementTypeOperandAsUse() const;
  unsigned getNumberOfDimensions() const;
  llvm::Value &getNumberOfDimensionsOperand() const;
  llvm::Use &getNumberOfDimensionsOperandAsUse() const;
  size_t getLengthOfDimension(unsigned dimension_index) const;
  llvm::Value &getLengthOfDimensionOperand(unsigned dimension_index) const;
  llvm::Use &getLengthOfDimensionOperandAsUse(unsigned dimension_index) const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::STATIC_TENSOR_TYPE);
  };

  std::string toString(std::string indent = "") const override;

protected:
  StaticTensorTypeInst(llvm::CallInst &call_inst);

  friend class MemOIRInst;
};

struct TensorTypeInst : public TypeInst {
public:
  Type &getType() const override;
  Type &getElementType() const;
  llvm::Value &getElementOperand() const;
  llvm::Use &getElementOperandAsUse() const;
  unsigned getNumberOfDimensions() const;
  llvm::Value &getNumberOfDimensionsOperand() const;
  llvm::Use &getNumberOfDimensionsOperandAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::TENSOR_TYPE);
  };

  std::string toString(std::string indent = "") const override;

protected:
  TensorTypeInst(llvm::CallInst &call_inst);

  friend class MemOIRInst;
};

struct AssocArrayTypeInst : public TypeInst {
public:
  Type &getType() const override;
  Type &getKeyType() const;
  llvm::Value &getKeyOperand() const;
  llvm::Use &getKeyOperandAsUse() const;
  Type &getValueType() const;
  llvm::Value &getValueOperand() const;
  llvm::Use &getValueOperandAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::ASSOC_ARRAY_TYPE);
  };

  std::string toString(std::string indent = "") const override;

protected:
  AssocArrayTypeInst(llvm::CallInst &call_inst);

  friend class MemOIRInst;
};

struct SequenceTypeInst : public TypeInst {
public:
  Type &getType() const override;
  Type &getElementType() const;
  llvm::Value &getElementOperand() const;
  llvm::Use &getElementOperandAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::SEQUENCE_TYPE);
  };

  std::string toString(std::string indent = "") const override;

protected:
  SequenceTypeInst(llvm::CallInst &call_inst);

  friend class MemOIRInst;
};

/*
 * Allocations
 */
struct AllocInst : public MemOIRInst {
public:
  llvm::Value &getAllocation() const;
  virtual Type &getType() const = 0;

  static bool classof(const MemOIRInst *I) {
    return
#define HANDLE_ALLOC_INST(ENUM, FUNC, CLASS)                                   \
  (I->getKind() == MemOIR_Func::ENUM) ||
#include "memoir/ir/Instructions.def"
        false;
  };

protected:
  AllocInst(llvm::CallInst &call_inst);

  friend class MemOIRInst;
};

struct StructAllocInst : public AllocInst {
public:
  Struct &getStruct() const;

  StructType &getStructType() const;
  Type &getType() const override;
  llvm::Value &getTypeOperand() const;
  llvm::Use &getTypeOperandAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::ALLOCATE_STRUCT);
  };

  std::string toString(std::string indent = "") const override;

protected:
  StructAllocInst(llvm::CallInst &call_inst);

  friend class MemOIRInst;
};

struct CollectionAllocInst : public AllocInst {
public:
  virtual Collection &getCollection() const = 0;
  virtual CollectionType &getCollectionType() const = 0;

  static bool classof(const MemOIRInst *I) {
    return (
#define HANDLE_COLLECTION_ALLOC_INST(ENUM, FUNC, CLASS)                        \
  (I->getKind() == MemOIR_Func::ENUM) ||
#include "memoir/ir/Instructions.def"
        false);
  };

  Type &getType() const override;

protected:
  CollectionAllocInst(llvm::CallInst &call_inst);

  friend class MemOIRInst;
};

struct TensorAllocInst : public CollectionAllocInst {
public:
  Collection &getCollection() const override;
  CollectionType &getCollectionType() const override;

  Type &getElementType() const;
  llvm::Value &getElementOperand() const;
  llvm::Use &getElementOperandAsUse() const;
  unsigned getNumberOfDimensions() const;
  llvm::Value &getNumberOfDimensionsOperand() const;
  llvm::Use &getNumberOfDimensionsOperandAsUse() const;
  llvm::Value &getLengthOfDimensionOperand(unsigned dimension_index) const;
  llvm::Use &getLengthOfDimensionOperandAsUse(unsigned dimension_index) const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::ALLOCATE_TENSOR);
  };

  std::string toString(std::string indent = "") const override;

protected:
  TensorAllocInst(llvm::CallInst &call_inst);

  friend class MemOIRInst;
};

struct AssocArrayAllocInst : public CollectionAllocInst {
public:
  Collection &getCollection() const override;
  CollectionType &getCollectionType() const override;

  Type &getKeyType() const;
  llvm::Value &getKeyOperand() const;
  llvm::Use &getKeyOperandAsUse() const;

  Type &getValueType() const;
  llvm::Value &getValueOperand() const;
  llvm::Use &getValueOperandAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::ALLOCATE_ASSOC_ARRAY);
  };

  std::string toString(std::string indent = "") const override;

protected:
  AssocArrayAllocInst(llvm::CallInst &call_inst);

  friend class MemOIRInst;
};

struct SequenceAllocInst : public CollectionAllocInst {
public:
  Collection &getCollection() const override;
  CollectionType &getCollectionType() const override;

  Type &getElementType() const;
  llvm::Value &getElementOperand() const;
  llvm::Use &getElementOperandAsUse() const;

  llvm::Value &getSizeOperand() const;
  llvm::Use &getSizeOperandAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::ALLOCATE_SEQUENCE);
  };

  std::string toString(std::string indent = "") const override;

protected:
  SequenceAllocInst(llvm::CallInst &call_inst);

  friend class MemOIRInst;
};

/*
 * Accesses
 */
struct AccessInst : public MemOIRInst {
public:
  virtual Collection &getCollectionAccessed() const = 0;
  virtual CollectionType &getCollectionType() const = 0;

  virtual llvm::Value &getObjectOperand() const = 0;
  virtual llvm::Use &getObjectOperandAsUse() const = 0;

  static bool classof(const MemOIRInst *I) {
    return (
#define HANDLE_ACCESS_INST(ENUM, FUNC, CLASS)                                  \
  (I->getKind() == MemOIR_Func::ENUM) ||
#include "memoir/ir/Instructions.def"
        false);
  };

protected:
  AccessInst(llvm::CallInst &call_inst);

  friend class MemOIRInst;
};

/*
 * Read Accesses
 */
struct ReadInst : public AccessInst {
public:
  CollectionType &getCollectionType() const override;

  llvm::Value &getValueRead() const;
  llvm::Value &getObjectOperand() const override;
  llvm::Use &getObjectOperandAsUse() const override;

  static bool classof(const MemOIRInst *I) {
    return
#define HANDLE_READ_INST(ENUM, FUNC, CLASS)                                    \
  (I->getKind() == MemOIR_Func::ENUM) ||
#include "memoir/ir/Instructions.def"
        false;
  };

protected:
  ReadInst(llvm::CallInst &call_inst);

  friend class MemOIRInst;
};

struct StructReadInst : public ReadInst {
public:
  Collection &getCollectionAccessed() const override;
  CollectionType &getCollectionType() const override;
  Struct &getStructAccessed() const;

  unsigned getFieldIndex() const;
  llvm::Value &getFieldIndexOperand() const;
  llvm::Use &getFieldIndexOperandAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return
#define HANDLE_STRUCT_READ_INST(ENUM, FUNC, CLASS)                             \
  (I->getKind() == MemOIR_Func::ENUM) ||
#include "memoir/ir/Instructions.def"
        false;
  };

  std::string toString(std::string indent = "") const override;

protected:
  StructReadInst(llvm::CallInst &call_inst);

  friend class MemOIRInst;
};

struct IndexReadInst : public ReadInst {
public:
  Collection &getCollectionAccessed() const override;

  unsigned getNumberOfDimensions() const;
  llvm::Value &getIndexOfDimension(unsigned dim_idx) const;
  llvm::Use &getIndexOfDimensionAsUse(unsigned dim_idx) const;

  static bool classof(const MemOIRInst *I) {
    return
#define HANDLE_INDEX_READ_INST(ENUM, FUNC, CLASS)                              \
  (I->getKind() == MemOIR_Func::ENUM) ||
#include "memoir/ir/Instructions.def"
        false;
  };

  std::string toString(std::string indent = "") const override;

protected:
  IndexReadInst(llvm::CallInst &call_inst);

  friend class MemOIRInst;
};

struct AssocReadInst : public ReadInst {
public:
  Collection &getCollectionAccessed() const override;

  llvm::Value &getKeyOperand() const;
  llvm::Use &getKeyOperandAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return
#define HANDLE_ASSOC_READ_INST(ENUM, FUNC, CLASS)                              \
  (I->getKind() == MemOIR_Func::ENUM) ||
#include "memoir/ir/Instructions.def"
        false;
  };

  std::string toString(std::string indent = "") const override;

protected:
  AssocReadInst(llvm::CallInst &call_inst);

  friend class MemOIRInst;
};

/*
 * Write Accesses
 */
struct WriteInst : public AccessInst {
public:
  CollectionType &getCollectionType() const override;

  llvm::Value &getValueWritten() const;
  llvm::Use &getValueWrittenAsUse() const;

  llvm::Value &getObjectOperand() const override;
  llvm::Use &getObjectOperandAsUse() const override;

  static bool classof(const MemOIRInst *I) {
    return
#define HANDLE_WRITE_INST(ENUM, FUNC, CLASS)                                   \
  (I->getKind() == MemOIR_Func::ENUM) ||
#include "memoir/ir/Instructions.def"
        false;
  };

protected:
  WriteInst(llvm::CallInst &call_inst);

  friend class MemOIRInst;
};

struct StructWriteInst : public WriteInst {
public:
  Collection &getCollectionAccessed() const override;
  CollectionType &getCollectionType() const override;

  Struct &getStructAccessed() const;

  unsigned getFieldIndex() const;
  llvm::Value &getFieldIndexOperand() const;
  llvm::Use &getFieldIndexOperandAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return
#define HANDLE_STRUCT_WRITE_INST(ENUM, FUNC, CLASS)                            \
  (I->getKind() == MemOIR_Func::ENUM) ||
#include "memoir/ir/Instructions.def"
        false;
  };

  std::string toString(std::string indent = "") const override;

protected:
  StructWriteInst(llvm::CallInst &call_inst);

  friend class MemOIRInst;
};

struct IndexWriteInst : public WriteInst {
public:
  Collection &getCollectionAccessed() const override;

  unsigned getNumberOfDimensions() const;
  llvm::Value &getIndexOfDimension(unsigned dim_idx) const;
  llvm::Use &getIndexOfDimensionAsUse(unsigned dim_idx) const;

  static bool classof(const MemOIRInst *I) {
    return
#define HANDLE_INDEX_WRITE_INST(ENUM, FUNC, CLASS)                             \
  (I->getKind() == MemOIR_Func::ENUM) ||
#include "memoir/ir/Instructions.def"
        false;
  };

  std::string toString(std::string indent = "") const override;

protected:
  IndexWriteInst(llvm::CallInst &call_inst);

  friend class MemOIRInst;
};

struct AssocWriteInst : public WriteInst {
public:
  Collection &getCollectionAccessed() const override;

  llvm::Value &getKeyOperand() const;
  llvm::Use &getKeyOperandAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return
#define HANDLE_ASSOC_WRITE_INST(ENUM, FUNC, CLASS)                             \
  (I->getKind() == MemOIR_Func::ENUM) ||
#include "memoir/ir/Instructions.def"
        false;
  };

  std::string toString(std::string indent = "") const override;

protected:
  AssocWriteInst(llvm::CallInst &call_inst);

  friend class MemOIRInst;
};

/*
 * Nested Accesses
 */
struct GetInst : public AccessInst {
public:
  CollectionType &getCollectionType() const override;

  llvm::Value &getValueRead() const;
  llvm::Value &getObjectOperand() const override;
  llvm::Use &getObjectOperandAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return
#define HANDLE_GET_INST(ENUM, FUNC, CLASS)                                     \
  (I->getKind() == MemOIR_Func::ENUM) ||
#include "memoir/ir/Instructions.def"
        false;
  };

protected:
  GetInst(llvm::CallInst &call_inst);

  friend class MemOIRInst;
};

struct StructGetInst : public GetInst {
public:
  Collection &getCollectionAccessed() const override;
  CollectionType &getCollectionType() const override;

  Struct &getStructAccessed() const;

  unsigned getFieldIndex() const;
  llvm::Value &getFieldIndexOperand() const;
  llvm::Use &getFieldIndexOperandAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return
#define HANDLE_STRUCT_GET_INST(ENUM, FUNC, CLASS)                              \
  (I->getKind() == MemOIR_Func::ENUM) ||
#include "memoir/ir/Instructions.def"
        false;
  };

  std::string toString(std::string indent = "") const override;

protected:
  StructGetInst(llvm::CallInst &call_inst);

  friend class MemOIRInst;
};

struct IndexGetInst : public GetInst {
public:
  Collection &getCollectionAccessed() const override;

  unsigned getNumberOfDimensions() const;
  llvm::Value &getIndexOfDimension(unsigned dim_idx) const;
  llvm::Use &getIndexOfDimensionAsUse(unsigned dim_idx) const;

  static bool classof(const MemOIRInst *I) {
    return
#define HANDLE_INDEX_GET_INST(ENUM, FUNC, CLASS)                               \
  (I->getKind() == MemOIR_Func::ENUM) ||
#include "memoir/ir/Instructions.def"
        false;
  };

  std::string toString(std::string indent = "") const override;

protected:
  IndexGetInst(llvm::CallInst &call_inst);

  friend class MemOIRInst;
};

struct AssocGetInst : public GetInst {
public:
  Collection &getCollectionAccessed() const override;

  llvm::Value &getKeyOperand() const;
  llvm::Use &getKeyOperandAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return
#define HANDLE_ASSOC_GET_INST(ENUM, FUNC, CLASS)                               \
  (I->getKind() == MemOIR_Func::ENUM) ||
#include "memoir/ir/Instructions.def"
        false;
  };

  std::string toString(std::string indent = "") const override;

protected:
  AssocGetInst(llvm::CallInst &call_inst);

  friend class MemOIRInst;
};

/*
 * Collection operations
 */
struct DeleteStructInst : public MemOIRInst {
public:
  Struct &getStructDeleted() const;
  llvm::Value &getStructOperand() const;
  llvm::Use &getStructOperandAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::DELETE_STRUCT);
  };

  std::string toString(std::string indent = "") const override;

protected:
  DeleteStructInst(llvm::CallInst &call_inst);

  friend class MemOIRInst;
};

struct DeleteCollectionInst : public MemOIRInst {
public:
  Collection &getCollectionDeleted() const;
  llvm::Value &getCollectionOperand() const;
  llvm::Use &getCollectionOperandAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::DELETE_COLLECTION);
  };

  std::string toString(std::string indent = "") const override;

protected:
  DeleteCollectionInst(llvm::CallInst &call_inst);

  friend class MemOIRInst;
};

struct JoinInst : public MemOIRInst {
public:
  Collection &getCollection() const;

  unsigned getNumberOfJoins() const;
  llvm::Value &getNumberOfJoinsOperand() const;
  llvm::Use &getNumberOfJoinsOperandAsUse() const;

  Collection &getJoinedCollection(unsigned join_index) const;
  llvm::Value &getJoinedOperand(unsigned join_index) const;
  llvm::Use &getJoinedOperandAsUse(unsigned join_index) const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::JOIN);
  };

  std::string toString(std::string indent = "") const override;

protected:
  JoinInst(llvm::CallInst &call_inst);

  friend class MemOIRInst;
};

struct SliceInst : public MemOIRInst {
public:
  Collection &getSlice() const;
  llvm::Value &getSliceAsValue() const;

  Collection &getCollection() const;
  llvm::Value &getCollectionOperand() const;
  llvm::Use &getCollectionOperandAsUse() const;

  llvm::Value &getBeginIndex() const;
  llvm::Use &getBeginIndexAsUse() const;

  llvm::Value &getEndIndex() const;
  llvm::Use &getEndIndexAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::SLICE);
  };

  std::string toString(std::string indent = "") const override;

protected:
  SliceInst(llvm::CallInst &call_inst);

  friend class MemOIRInst;
};

/*
 * Type checking
 */
struct AssertStructTypeInst : public MemOIRInst {
public:
  Type &getType() const;
  llvm::Value &getTypeOperand() const;
  llvm::Use &getTypeOperandAsUse() const;

  Struct &getStruct() const;
  llvm::Value &getStructOperand() const;
  llvm::Use &getStructOperandAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::ASSERT_STRUCT_TYPE);
  };

  std::string toString(std::string indent = "") const override;

protected:
  AssertStructTypeInst(llvm::CallInst &call_inst);

  friend class MemOIRInst;
};

struct AssertCollectionTypeInst : public MemOIRInst {
public:
  Type &getType() const;
  llvm::Value &getTypeOperand() const;
  llvm::Use &getTypeOperandAsUse() const;

  Collection &getCollection() const;
  llvm::Value &getCollectionOperand() const;
  llvm::Use &getCollectionOperandAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::ASSERT_COLLECTION_TYPE);
  };

  std::string toString(std::string indent = "") const override;

protected:
  AssertCollectionTypeInst(llvm::CallInst &call_inst);

  friend class MemOIRInst;
};

struct ReturnTypeInst : public MemOIRInst {
public:
  Type &getType() const;
  llvm::Value &getTypeOperand() const;
  llvm::Use &getTypeOperandAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::SET_RETURN_TYPE);
  };

  std::string toString(std::string indent = "") const override;

protected:
  ReturnTypeInst(llvm::CallInst &call_inst);

  friend class MemOIRInst;
};

} // namespace llvm::memoir

#endif
