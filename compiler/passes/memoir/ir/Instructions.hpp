#ifndef COMMON_INSTRUCTIONS_H
#define COMMON_INSTRUCTIONS_H
#pragma once

#include <cstddef>

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

#include "memoir/support/Casting.hpp"

#include "memoir/utility/FunctionNames.hpp"

#include "memoir/ir/Function.hpp"
#include "memoir/ir/Types.hpp"

/*
 * MemOIR Instructions and a wrapper of an LLVM Instruction.
 *
 * Author(s): Tommy McMichen
 * Created: December 13, 2022
 */

namespace llvm::memoir {

struct MemOIRFunction;
struct CollectionType;

// Abstract MemOIR Instruction
struct MemOIRInst {
public:
  static MemOIRInst *get(llvm::Instruction &I);
  static bool is_mutator(MemOIRInst &I);
  static void invalidate();

  MemOIRFunction &getFunction() const;
  llvm::CallInst &getCallInst() const;
  llvm::Function &getLLVMFunction() const;
  llvm::Module *getModule() const;
  llvm::BasicBlock *getParent() const;
  MemOIR_Func getKind() const;

  explicit operator llvm::Value *() {
    return &this->getCallInst();
  }
  explicit operator llvm::Value &() {
    return this->getCallInst();
  }
  explicit operator llvm::Instruction *() {
    return &this->getCallInst();
  }
  explicit operator llvm::Instruction &() {
    return this->getCallInst();
  }

  friend std::ostream &operator<<(std::ostream &os, const MemOIRInst &I);
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                       const MemOIRInst &I);
  virtual std::string toString(std::string indent = "") const = 0;

protected:
  llvm::CallInst &call_inst;

  static map<llvm::Instruction *, MemOIRInst *> *llvm_to_memoir;

  MemOIRInst(llvm::CallInst &call_inst) : call_inst(call_inst){};
};

// Types.
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
  TypeInst(llvm::CallInst &call_inst) : MemOIRInst(call_inst){};

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
  UInt64TypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst){};

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
  UInt32TypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst){};

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
  UInt16TypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst){};

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
  UInt8TypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst){};

  friend class MemOIRInst;
};

struct UInt2TypeInst : public TypeInst {
public:
  Type &getType() const override;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::UINT2_TYPE);
  };

  std::string toString(std::string indent = "") const override;

protected:
  UInt2TypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst){};

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
  Int64TypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst){};

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
  Int32TypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst){};

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
  Int16TypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst){};

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
  Int8TypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst){};

  friend class MemOIRInst;
};

struct Int2TypeInst : public TypeInst {
public:
  Type &getType() const override;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::INT2_TYPE);
  };

  std::string toString(std::string indent = "") const override;

protected:
  Int2TypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst){};

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
  BoolTypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst){};

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
  FloatTypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst){};

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
  DoubleTypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst){};

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
  PointerTypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst){};

  friend class MemOIRInst;
};

struct ReferenceTypeInst : public TypeInst {
public:
  Type &getType() const override;
  Type &getReferencedType() const;
  llvm::Value &getReferencedTypeOperand() const;
  llvm::Use &getReferencedTypeOperandAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::REFERENCE_TYPE);
  };

  std::string toString(std::string indent = "") const override;

protected:
  ReferenceTypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst){};

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
  DefineStructTypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst){};

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
  StructTypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst){};

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
  StaticTensorTypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst){};

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
  TensorTypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst){};

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
  AssocArrayTypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst){};

  friend class MemOIRInst;
};
using AssocTypeInst = struct AssocArrayTypeInst;

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
  SequenceTypeInst(llvm::CallInst &call_inst) : TypeInst(call_inst){};

  friend class MemOIRInst;
};

// Allocations
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
  AllocInst(llvm::CallInst &call_inst) : MemOIRInst(call_inst){};

  friend class MemOIRInst;
};

struct StructAllocInst : public AllocInst {
public:
  llvm::Value &getStruct() const;

  StructType &getStructType() const;
  Type &getType() const override;

  llvm::Value &getTypeOperand() const;
  llvm::Use &getTypeOperandAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::ALLOCATE_STRUCT);
  };

  std::string toString(std::string indent = "") const override;

protected:
  StructAllocInst(llvm::CallInst &call_inst) : AllocInst(call_inst){};

  friend class MemOIRInst;
};

struct CollectionAllocInst : public AllocInst {
public:
  virtual llvm::Value &getCollection() const = 0;
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
  CollectionAllocInst(llvm::CallInst &call_inst) : AllocInst(call_inst){};

  friend class MemOIRInst;
};

struct TensorAllocInst : public CollectionAllocInst {
public:
  llvm::Value &getCollection() const override;
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
  TensorAllocInst(llvm::CallInst &call_inst) : CollectionAllocInst(call_inst){};

  friend class MemOIRInst;
};

struct AssocArrayAllocInst : public CollectionAllocInst {
public:
  llvm::Value &getCollection() const override;
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
  AssocArrayAllocInst(llvm::CallInst &call_inst)
    : CollectionAllocInst(call_inst){};

  friend class MemOIRInst;
};
using AssocAllocInst = struct AssocArrayAllocInst;

struct SequenceAllocInst : public CollectionAllocInst {
public:
  llvm::Value &getCollection() const override;
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
  SequenceAllocInst(llvm::CallInst &call_inst)
    : CollectionAllocInst(call_inst){};

  friend class MemOIRInst;
};

// Accesses
struct AccessInst : public MemOIRInst {
public:
  virtual CollectionType &getCollectionType() const;

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
  AccessInst(llvm::CallInst &call_inst) : MemOIRInst(call_inst){};

  friend class MemOIRInst;
};

// Read Accesses
struct ReadInst : public AccessInst {
public:
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
  ReadInst(llvm::CallInst &call_inst) : AccessInst(call_inst){};

  friend class MemOIRInst;
};

struct StructReadInst : public ReadInst {
public:
  CollectionType &getCollectionType() const override;

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
  StructReadInst(llvm::CallInst &call_inst) : ReadInst(call_inst){};

  friend class MemOIRInst;
};

struct IndexReadInst : public ReadInst {
public:
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
  IndexReadInst(llvm::CallInst &call_inst) : ReadInst(call_inst){};

  friend class MemOIRInst;
};

struct AssocReadInst : public ReadInst {
public:
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
  AssocReadInst(llvm::CallInst &call_inst) : ReadInst(call_inst){};

  friend class MemOIRInst;
};

// Write Accesses
struct WriteInst : public AccessInst {
public:
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
  WriteInst(llvm::CallInst &call_inst) : AccessInst(call_inst){};

  friend class MemOIRInst;
};

struct StructWriteInst : public WriteInst {
public:
  CollectionType &getCollectionType() const override;

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
  StructWriteInst(llvm::CallInst &call_inst) : WriteInst(call_inst){};

  friend class MemOIRInst;
};

struct IndexWriteInst : public WriteInst {
public:
  llvm::Value &getCollection() const;

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
  IndexWriteInst(llvm::CallInst &call_inst) : WriteInst(call_inst){};

  friend class MemOIRInst;
};

struct AssocWriteInst : public WriteInst {
public:
  llvm::Value &getCollection() const;

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
  AssocWriteInst(llvm::CallInst &call_inst) : WriteInst(call_inst){};

  friend class MemOIRInst;
};

// Nested Accesses
struct GetInst : public AccessInst {
public:
  llvm::Value &getNestedObject() const;

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
  GetInst(llvm::CallInst &call_inst) : AccessInst(call_inst){};

  friend class MemOIRInst;
};

struct StructGetInst : public GetInst {
public:
  CollectionType &getCollectionType() const override;

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
  StructGetInst(llvm::CallInst &call_inst) : GetInst(call_inst){};

  friend class MemOIRInst;
};

struct IndexGetInst : public GetInst {
public:
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
  IndexGetInst(llvm::CallInst &call_inst) : GetInst(call_inst){};

  friend class MemOIRInst;
};

struct AssocGetInst : public GetInst {
public:
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
  AssocGetInst(llvm::CallInst &call_inst) : GetInst(call_inst){};

  friend class MemOIRInst;
};

// Abstract insert operation.
struct InsertInst : public MemOIRInst {
public:
  virtual llvm::Value &getResultCollection() const;

  virtual llvm::Value &getBaseCollection() const = 0;
  virtual llvm::Use &getBaseCollectionAsUse() const = 0;

  virtual llvm::Value &getInsertionPoint() const = 0;
  virtual llvm::Use &getInsertionPointAsUse() const = 0;

  static bool classof(const MemOIRInst *I) {
    return
#define HANDLE_INSERT_INST(ENUM, FUNC, CLASS)                                  \
  (I->getKind() == MemOIR_Func::ENUM) ||
#include "memoir/ir/Instructions.def"
        false;
  }

  InsertInst(llvm::CallInst &call_inst) : MemOIRInst(call_inst) {}
};

// Sequence insert operations.
struct SeqInsertInst : public InsertInst {
public:
  llvm::Value &getBaseCollection() const override;
  llvm::Use &getBaseCollectionAsUse() const override;

  llvm::Value &getValueInserted() const;
  llvm::Use &getValueInsertedAsUse() const;

  llvm::Value &getInsertionPoint() const override;
  llvm::Use &getInsertionPointAsUse() const override;

  static bool classof(const MemOIRInst *I) {
    return
#define HANDLE_SEQ_INSERT_INST(ENUM, FUNC, CLASS)                              \
  (I->getKind() == MemOIR_Func::ENUM) ||
#include "memoir/ir/Instructions.def"
        false;
  };

  std::string toString(std::string indent = "") const override;

protected:
  SeqInsertInst(llvm::CallInst &call_inst) : InsertInst(call_inst) {}

  friend class MemOIRInst;
};

struct SeqInsertSeqInst : public InsertInst {
public:
  llvm::Value &getBaseCollection() const override;
  llvm::Use &getBaseCollectionAsUse() const override;

  llvm::Value &getInsertedCollection() const;
  llvm::Use &getInsertedCollectionAsUse() const;

  llvm::Value &getInsertionPoint() const override;
  llvm::Use &getInsertionPointAsUse() const override;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::SEQ_INSERT);
  };

  std::string toString(std::string indent = "") const override;

protected:
  SeqInsertSeqInst(llvm::CallInst &call_inst) : InsertInst(call_inst) {}

  friend class MemOIRInst;
};

struct RemoveInst : public MemOIRInst {
public:
  virtual llvm::Value &getResultCollection() const;

  virtual llvm::Value &getBaseCollection() const = 0;
  virtual llvm::Use &getBaseCollectionAsUse() const = 0;

  static bool classof(const MemOIRInst *I) {
    return
#define HANDLE_REMOVE_INST(ENUM, FUNC, CLASS)                                  \
  (I->getKind() == MemOIR_Func::ENUM) ||
#include "memoir/ir/Instructions.def"
        false;
  };

  RemoveInst(llvm::CallInst &call_inst) : MemOIRInst(call_inst) {}
};

struct SeqRemoveInst : public RemoveInst {
public:
  llvm::Value &getBaseCollection() const override;
  llvm::Use &getBaseCollectionAsUse() const override;

  llvm::Value &getBeginIndex() const;
  llvm::Use &getBeginIndexAsUse() const;

  llvm::Value &getEndIndex() const;
  llvm::Use &getEndIndexAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::SEQ_REMOVE);
  };

  std::string toString(std::string indent = "") const override;

protected:
  SeqRemoveInst(llvm::CallInst &call_inst) : RemoveInst(call_inst){};

  friend class MemOIRInst;
};

struct SwapInst : public MemOIRInst {
  llvm::Value &getResult() const;

  virtual llvm::Value &getIncomingCollectionFor(
      llvm::Value &collection) const = 0;
  virtual llvm::Use &getIncomingCollectionForAsUse(
      llvm::Value &collection) const = 0;

  virtual llvm::Value &getFromCollection() const = 0;
  virtual llvm::Use &getFromCollectionAsUse() const = 0;

  virtual llvm::Value &getBeginIndex() const = 0;
  virtual llvm::Use &getBeginIndexAsUse() const = 0;

  virtual llvm::Value &getEndIndex() const = 0;
  virtual llvm::Use &getEndIndexAsUse() const = 0;

  virtual llvm::Value &getToCollection() const = 0;
  virtual llvm::Use &getToCollectionAsUse() const = 0;

  virtual llvm::Value &getToBeginIndex() const = 0;
  virtual llvm::Use &getToBeginIndexAsUse() const = 0;

  static bool classof(MemOIRInst *I) {
    return
#define HANDLE_SWAP_INST(ENUM, FUNC, CLASS)                                    \
  (I->getKind() == MemOIR_Func::ENUM) ||
#include "memoir/ir/Instructions.def"
        false;
  }

  SwapInst(llvm::CallInst &call_inst) : MemOIRInst(call_inst) {}
};

struct SeqSwapInst : public SwapInst {
public:
  llvm::Value &getIncomingCollectionFor(llvm::Value &collection) const override;
  llvm::Use &getIncomingCollectionForAsUse(
      llvm::Value &collection) const override;

  llvm::Value &getFromCollection() const override;
  llvm::Use &getFromCollectionAsUse() const override;

  llvm::Value &getBeginIndex() const override;
  llvm::Use &getBeginIndexAsUse() const override;

  llvm::Value &getEndIndex() const override;
  llvm::Use &getEndIndexAsUse() const override;

  llvm::Value &getToCollection() const override;
  llvm::Use &getToCollectionAsUse() const override;

  llvm::Value &getToBeginIndex() const override;
  llvm::Use &getToBeginIndexAsUse() const override;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::SEQ_SWAP);
  };

  std::string toString(std::string indent = "") const override;

protected:
  SeqSwapInst(llvm::CallInst &call_inst) : SwapInst(call_inst){};

  friend class MemOIRInst;
};

struct SeqSwapWithinInst : public SwapInst {
public:
  llvm::Value &getIncomingCollectionFor(llvm::Value &collection) const override;
  llvm::Use &getIncomingCollectionForAsUse(
      llvm::Value &collection) const override;

  llvm::Value &getFromCollection() const override;
  llvm::Use &getFromCollectionAsUse() const override;

  llvm::Value &getBeginIndex() const override;
  llvm::Use &getBeginIndexAsUse() const override;

  llvm::Value &getEndIndex() const override;
  llvm::Use &getEndIndexAsUse() const override;

  llvm::Value &getToCollection() const override;
  llvm::Use &getToCollectionAsUse() const override;

  llvm::Value &getToBeginIndex() const override;
  llvm::Use &getToBeginIndexAsUse() const override;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::SEQ_SWAP_WITHIN);
  };

  std::string toString(std::string indent = "") const override;

protected:
  SeqSwapWithinInst(llvm::CallInst &call_inst) : SwapInst(call_inst) {}

  friend class MemOIRInst;
};

struct CopyInst : public MemOIRInst {
  virtual llvm::Value &getCopy() const;

  virtual llvm::Value &getCopiedCollection() const;
  virtual llvm::Use &getCopiedCollectionAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return
#define HANDLE_COPY_INST(ENUM, FUNC, CLASS)                                    \
  (I->getKind() == MemOIR_Func::ENUM) ||
#include "memoir/ir/Instructions.def"
        false;
  }

protected:
  CopyInst(llvm::CallInst &call_inst) : MemOIRInst(call_inst){};

  friend class MemOIRInst;
};

struct SeqCopyInst : public CopyInst {
public:
  llvm::Value &getBeginIndex() const;
  llvm::Use &getBeginIndexAsUse() const;

  llvm::Value &getEndIndex() const;
  llvm::Use &getEndIndexAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::SEQ_COPY);
  };

  std::string toString(std::string indent = "") const override;

protected:
  SeqCopyInst(llvm::CallInst &call_inst) : CopyInst(call_inst){};

  friend class MemOIRInst;
};

// Other sequence operations.
struct SizeInst : public MemOIRInst {
public:
  llvm::Value &getSize() const;

  llvm::Value &getCollection() const;
  llvm::Use &getCollectionAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::SIZE);
  };

  std::string toString(std::string indent = "") const override;

protected:
  SizeInst(llvm::CallInst &call_inst) : MemOIRInst(call_inst){};

  friend class MemOIRInst;
};

struct EndInst : public MemOIRInst {
public:
  llvm::Value &getValue() const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::END);
  };

  std::string toString(std::string indent = "") const override;

protected:
  EndInst(llvm::CallInst &call_inst) : MemOIRInst(call_inst){};

  friend class MemOIRInst;
};

// Assoc operations.
struct AssocHasInst : public AccessInst {
public:
  llvm::Value &getObjectOperand() const override;
  llvm::Use &getObjectOperandAsUse() const override;

  llvm::Value &getKeyOperand() const;
  llvm::Use &getKeyOperandAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::ASSOC_HAS);
  };

  std::string toString(std::string indent = "") const override;

protected:
  AssocHasInst(llvm::CallInst &call_inst) : AccessInst(call_inst){};

  friend class MemOIRInst;
};

struct AssocInsertInst : public InsertInst {
public:
  llvm::Value &getBaseCollection() const override;
  llvm::Use &getBaseCollectionAsUse() const override;

  llvm::Value &getInsertionPoint() const override;
  llvm::Use &getInsertionPointAsUse() const override;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::ASSOC_INSERT);
  };

  std::string toString(std::string indent = "") const override;

protected:
  AssocInsertInst(llvm::CallInst &call_inst) : InsertInst(call_inst){};

  friend class MemOIRInst;
};

struct AssocRemoveInst : public RemoveInst {
public:
  llvm::Value &getBaseCollection() const override;
  llvm::Use &getBaseCollectionAsUse() const override;

  llvm::Value &getKey() const;
  llvm::Use &getKeyAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::ASSOC_REMOVE);
  };

  std::string toString(std::string indent = "") const override;

protected:
  AssocRemoveInst(llvm::CallInst &call_inst) : RemoveInst(call_inst){};

  friend class MemOIRInst;
};

struct AssocKeysInst : public MemOIRInst {
public:
  llvm::Value &getKeys() const;

  llvm::Value &getCollection() const;
  llvm::Use &getCollectionAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::ASSOC_KEYS);
  };

  std::string toString(std::string indent = "") const override;

protected:
  AssocKeysInst(llvm::CallInst &call_inst) : MemOIRInst(call_inst){};

  friend class MemOIRInst;
};

// SSA/readonce operations.
struct UsePHIInst : public MemOIRInst {
public:
  llvm::Value &getResultCollection() const;

  llvm::Value &getUsedCollection() const;
  llvm::Use &getUsedCollectionAsUse() const;

  llvm::Instruction &getUseInst() const;
  void setUseInst(llvm::Instruction &I) const;
  void setUseInst(MemOIRInst &I) const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::USE_PHI);
  };

  std::string toString(std::string indent = "") const override;

protected:
  UsePHIInst(llvm::CallInst &call_inst) : MemOIRInst(call_inst){};

  friend class MemOIRInst;
};

struct DefPHIInst : public MemOIRInst {
public:
  llvm::Value &getResultCollection() const;

  llvm::Value &getDefinedCollection() const;
  llvm::Use &getDefinedCollectionAsUse() const;

  llvm::Instruction &getDefInst() const;
  void setDefInst(llvm::Instruction &I) const;
  void setDefInst(MemOIRInst &I) const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::DEF_PHI);
  };

  std::string toString(std::string indent = "") const override;

protected:
  DefPHIInst(llvm::CallInst &call_inst) : MemOIRInst(call_inst){};

  friend class MemOIRInst;
};

struct ArgPHIInst : public MemOIRInst {
public:
  llvm::Value &getResultCollection() const;

  llvm::Value &getInputCollection() const;
  llvm::Use &getInputCollectionAsUse() const;

  // TODO: add methods for decoding the metadata

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::ARG_PHI);
  };

  std::string toString(std::string indent = "") const override;

protected:
  ArgPHIInst(llvm::CallInst &call_inst) : MemOIRInst(call_inst){};

  friend class MemOIRInst;
};

struct RetPHIInst : public MemOIRInst {
public:
  llvm::Value &getResultCollection() const;

  llvm::Value &getInputCollection() const;
  llvm::Use &getInputCollectionAsUse() const;

  // TODO: add methods for decoding the metadata.

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::RET_PHI);
  };

  std::string toString(std::string indent = "") const override;

protected:
  RetPHIInst(llvm::CallInst &call_inst) : MemOIRInst(call_inst){};

  friend class MemOIRInst;
};

/*
 * Lowering representations.
 */
struct ViewInst : public MemOIRInst {
public:
  llvm::Value &getView() const;

  llvm::Value &getCollection() const;
  llvm::Use &getCollectionAsUse() const;

  llvm::Value &getBeginIndex() const;
  llvm::Use &getBeginIndexAsUse() const;

  llvm::Value &getEndIndex() const;
  llvm::Use &getEndIndexAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::VIEW);
  };

  std::string toString(std::string indent = "") const override;

protected:
  ViewInst(llvm::CallInst &call_inst) : MemOIRInst(call_inst){};

  friend class MemOIRInst;
};

// Deletion operations
struct DeleteStructInst : public MemOIRInst {
public:
  llvm::Value &getDeletedStruct() const;
  llvm::Use &getDeletedStructAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::DELETE_STRUCT);
  };

  std::string toString(std::string indent = "") const override;

protected:
  DeleteStructInst(llvm::CallInst &call_inst) : MemOIRInst(call_inst){};

  friend class MemOIRInst;
};

struct DeleteCollectionInst : public MemOIRInst {
public:
  llvm::Value &getDeletedCollection() const;
  llvm::Use &getDeletedCollectionAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::DELETE_COLLECTION);
  };

  std::string toString(std::string indent = "") const override;

protected:
  DeleteCollectionInst(llvm::CallInst &call_inst) : MemOIRInst(call_inst){};

  friend class MemOIRInst;
};

// Type checking
struct AssertStructTypeInst : public MemOIRInst {
public:
  Type &getType() const;
  llvm::Value &getTypeOperand() const;
  llvm::Use &getTypeOperandAsUse() const;

  llvm::Value &getStruct() const;
  llvm::Use &getStructAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::ASSERT_STRUCT_TYPE);
  };

  std::string toString(std::string indent = "") const override;

protected:
  AssertStructTypeInst(llvm::CallInst &call_inst) : MemOIRInst(call_inst){};

  friend class MemOIRInst;
};

struct AssertCollectionTypeInst : public MemOIRInst {
public:
  Type &getType() const;
  llvm::Value &getTypeOperand() const;
  llvm::Use &getTypeOperandAsUse() const;

  llvm::Value &getCollection() const;
  llvm::Use &getCollectionAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::ASSERT_COLLECTION_TYPE);
  };

  std::string toString(std::string indent = "") const override;

protected:
  AssertCollectionTypeInst(llvm::CallInst &call_inst) : MemOIRInst(call_inst){};

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
  ReturnTypeInst(llvm::CallInst &call_inst) : MemOIRInst(call_inst){};

  friend class MemOIRInst;
};

// Functions to dyn_cast an llvm::Instruction to a MemOIRInst
template <
    class To,
    class From,
    std::enable_if_t<std::is_base_of_v<MemOIRInst, To>, bool> = true,
    std::enable_if_t<std::is_base_of_v<llvm::Instruction, From>, bool> = true>
To *as(From *I) {
  if (I == nullptr) {
    return nullptr;
  }
  auto *memoir_inst = MemOIRInst::get(*I);
  return dyn_cast_or_null<To>(memoir_inst);
}

template <
    class To,
    class From,
    std::enable_if_t<std::is_base_of_v<MemOIRInst, To>, bool> = true,
    std::enable_if_t<std::is_base_of_v<llvm::Instruction, From>, bool> = true>
To *as(From &I) {
  auto *memoir_inst = MemOIRInst::get(I);
  return dyn_cast_or_null<To>(memoir_inst);
}

} // namespace llvm::memoir

#endif
