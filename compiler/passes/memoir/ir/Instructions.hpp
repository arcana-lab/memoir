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

/*
 * Abstract MemOIR Instruction
 */
struct MemOIRInst {
public:
  static MemOIRInst *get(llvm::Instruction &I);

  MemOIRFunction &getFunction() const;
  llvm::CallInst &getCallInst() const;

  friend std::ostream &operator<<(std::ostream &os, const MemOIRInst &I);
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                       const MemOIRInst &I);
  virtual std::string toString(std::string indent = "") const = 0;

protected:
  MemOIR_Func memoir_enum;
  llvm::CallInst &call_inst;

  MemOIRInst(MemOIR_Func memoir_enum, llvm::CallInst &call_inst);

  static map<llvm::Instruction *, MemOIRInst *> llvm_to_memoir;
};

/*
 * Types
 */
struct TypeInst : public MemOIRInst {
public:
  virtual Type &getType() const = 0;

  std::string toString(std::string indent = "") const override;

protected:
  TypeInst(llvm::CallInst &call_inst);
};

struct UInt64TypeInst : public TypeInst {
public:
  Type &getType() const override;

  std::string toString(std::string indent = "") const override;

protected:
  UInt64TypeInst(llvm::CallInst &call_inst);
};

struct UInt32TypeInst : public TypeInst {
public:
  Type &getType() const override;

  std::string toString(std::string indent = "") const override;

protected:
  UInt32TypeInst(llvm::CallInst &call_inst);
};

struct UInt16TypeInst : public TypeInst {
public:
  Type &getType() const override;

  std::string toString(std::string indent = "") const override;

protected:
  UInt16TypeInst(llvm::CallInst &call_inst);
};

struct UInt8TypeInst : public TypeInst {
public:
  Type &getType() const override;

  std::string toString(std::string indent = "") const override;

protected:
  UInt8TypeInst(llvm::CallInst &call_inst);
};

struct Int64TypeInst : public TypeInst {
public:
  Type &getType() const override;

  std::string toString(std::string indent = "") const override;

protected:
  Int64TypeInst(llvm::CallInst &call_inst);
};

struct Int32TypeInst : public TypeInst {
public:
  Type &getType() const override;

  std::string toString(std::string indent = "") const override;

protected:
  Int32TypeInst(llvm::CallInst &call_inst);
};

struct Int16TypeInst : public TypeInst {
public:
  Type &getType() const override;

  std::string toString(std::string indent = "") const override;

protected:
  Int16TypeInst(llvm::CallInst &call_inst);
};

struct Int8TypeInst : public TypeInst {
public:
  Type &getType() const override;

  std::string toString(std::string indent = "") const override;

protected:
  Int8TypeInst(llvm::CallInst &call_inst);
};

struct BoolTypeInst : public TypeInst {
public:
  Type &getType() const override;

  std::string toString(std::string indent = "") const override;

protected:
  BoolTypeInst(llvm::CallInst &call_inst);
};

struct FloatTypeInst : public TypeInst {
public:
  Type &getType() const override;

  std::string toString(std::string indent = "") const override;

protected:
  FloatTypeInst(llvm::CallInst &call_inst);
};

struct DoubleTypeInst : public TypeInst {
public:
  Type &getType() const override;

  std::string toString(std::string indent = "") const override;

protected:
  DoubleTypeInst(llvm::CallInst &call_inst);
};

struct PointerTypeInst : public TypeInst {
public:
  Type &getType() const override;

  std::string toString(std::string indent = "") const override;

protected:
  PointerTypeInst(llvm::CallInst &call_inst);
};

struct ReferenceTypeInst : public TypeInst {
public:
  Type &getType() const override;
  Type &getReferencedType() const;
  llvm::Value &getReferencedTypeOperand() const;
  llvm::Use &getReferencedTypeAsUse() const;

  std::string toString(std::string indent = "") const override;

protected:
  ReferenceTypeInst(llvm::CallInst &call_inst);
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

  std::string toString(std::string indent = "") const override;

protected:
  DefineStructTypeInst(llvm::CallInst &call_inst);
};

struct StructTypeInst : public TypeInst {
public:
  Type &getType() const override;
  std::string getName() const;
  llvm::Value &getNameOperand() const;
  llvm::Use &getNameOperandAsUse() const;

  std::string toString(std::string indent = "") const override;

protected:
  StructTypeInst(llvm::CallInst &call_inst);
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
  llvm::Value &getNumberOfDimensionsOperand(unsigned dimension_index) const;
  llvm::Use &getNumberOfDimensionsOperandAsUse(unsigned dimension_index) const;

  std::string toString(std::string indent = "") const override;

protected:
  StaticTensorTypeInst(llvm::CallInst &call_inst);
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

  std::string toString(std::string indent = "") const override;

protected:
  TensorTypeInst(llvm::CallInst &call_inst);
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

  std::string toString(std::string indent = "") const override;

protected:
  AssocArrayTypeInst(llvm::CallInst &call_inst);
};

struct SequenceTypeInst : public TypeInst {
public:
  Type &getType() const override;
  Type &getElementType() const;
  llvm::Value &getElementOperand() const;
  llvm::Use &getElementOperandAsUse() const;

  std::string toString(std::string indent = "") const override;

protected:
  SequenceTypeInst(llvm::CallInst &call_inst);
};

/*
 * Allocations
 */
struct AllocInst : public MemOIRInst {
public:
  llvm::Value &getAllocation() const;
  virtual Type &getType() const = 0;

protected:
  AllocInst(llvm::CallInst &call_inst);
};

struct StructAllocInst : public AllocInst {
public:
  Struct &getStruct() const;

  StructType &getStructType() const;
  Type &getType() const override;
  llvm::Value &getTypeOperand() const;
  llvm::Use &getTypeOperandAsUse() const;

  std::string toString(std::string indent = "") const override;

protected:
  StructAllocInst(llvm::CallInst &call_inst);
};

struct CollectionAllocInst : public AllocInst {
public:
  virtual Collection &getCollection() const = 0;
  virtual CollectionType &getCollectionType() const = 0;

  Type &getType() const override;

protected:
  CollectionAllocInst(llvm::CallInst &call_inst);
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

  std::string toString(std::string indent = "") const override;

protected:
  TensorAllocInst(llvm::CallInst &call_inst);
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

  std::string toString(std::string indent = "") const override;

protected:
  AssocArrayAllocInst(llvm::CallInst &call_inst);
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

  std::string toString(std::string indent = "") const override;

protected:
  SequenceAllocInst(llvm::CallInst &call_inst);
};

/*
 * Accesses
 */
struct AccessInst : public MemOIRInst {
public:
  virtual Collection &getCollectionAccessed() const = 0;

  virtual llvm::Value &getObjectOperand() const = 0;
  virtual llvm::Use &getObjectOperandAsUse() const = 0;

protected:
  AccessInst(llvm::CallInst &call_inst);
};

/*
 * Read Accesses
 */
struct ReadInst : public AccessInst {
public:
  llvm::Value &getValueRead() const;
  llvm::Value &getObjectOperand() const override;
  llvm::Use &getObjectOperandAsUse() const override;

  std::string toString(std::string indent = "") const override;

protected:
  ReadInst(llvm::CallInst &call_inst);
};

struct StructReadInst : public ReadInst {
public:
  Collection &getCollectionAccessed() const override;

  FieldArray &getFieldArray() const;
  Struct &getStructAccessed() const;

  unsigned getFieldIndex() const;
  llvm::Value &getFieldIndexOperand() const;
  llvm::Use &getFieldIndexOperandAsUse() const;

  std::string toString(std::string indent = "") const override;

protected:
  StructReadInst(llvm::CallInst &call_inst);
};

struct IndexReadInst : public ReadInst {
public:
  Collection &getCollectionAccessed() const override;

  unsigned getNumberOfDimensions() const;
  llvm::Value &getIndexOfDimension(unsigned dim_idx) const;
  llvm::Use &getIndexOfDimensionAsUse(unsigned dim_idx) const;

  std::string toString(std::string indent = "") const override;

protected:
  IndexReadInst(llvm::CallInst &call_inst);
};

struct AssocReadInst : public ReadInst {
public:
  Collection &getCollectionAccessed() const override;

  llvm::Value &getKeyOperand() const;
  llvm::Use &getKeyOperandAsUse() const;

  std::string toString(std::string indent = "") const override;

protected:
  AssocReadInst(llvm::CallInst &call_inst);
};

/*
 * Write Accesses
 */
struct WriteInst : public AccessInst {
public:
  llvm::Value &getValueWritten() const;
  llvm::Use &getValueWrittenAsUse() const;
  llvm::Value &getObjectOperand() const override;
  llvm::Use &getObjectOperandAsUse() const;

  std::string toString(std::string indent = "") const override;

protected:
  WriteInst(llvm::CallInst &call_inst);
};

struct StructWriteInst : public WriteInst {
public:
  Collection &getCollectionAccessed() const override;

  FieldArray &getFieldArray() const;
  Struct &getStructAccessed() const;

  unsigned getFieldIndex() const;
  llvm::Value &getFieldIndexOperand() const;
  llvm::Use &getFieldIndexOperandAsUse() const;

  std::string toString(std::string indent = "") const override;

protected:
  StructWriteInst(llvm::CallInst &call_inst);
};

struct IndexWriteInst : public WriteInst {
public:
  Collection &getCollectionAccessed() const override;

  unsigned getNumberOfDimensions() const;
  llvm::Value &getIndexOfDimension(unsigned dim_idx) const;
  llvm::Use &getIndexOfDimensionAsUse(unsigned dim_idx) const;

  std::string toString(std::string indent = "") const override;

protected:
  IndexWriteInst(llvm::CallInst &call_inst);
};

struct AssocWriteInst : public WriteInst {
public:
  Collection &getCollectionAccessed() const override;

  llvm::Value &getKeyOperand() const;
  llvm::Use &getKeyOperandAsUse() const;

  std::string toString(std::string indent = "") const override;

protected:
  AssocWriteInst(llvm::CallInst &call_inst);
};

/*
 * Nested Accesses
 */
struct GetInst : public AccessInst {
public:
  llvm::Value &getValueRead() const;
  llvm::Value &getObjectOperand() const override;
  llvm::Use &getObjectOperandAsUse() const;

  std::string toString(std::string indent = "") const override;

protected:
  GetInst(llvm::CallInst &call_inst);
};

struct StructGetInst : public GetInst {
public:
  Collection &getCollectionAccessed() const override;

  unsigned getFieldIndex() const;
  llvm::Value &getFieldIndexOperand() const;
  llvm::Use &getFieldIndexOperandAsUse() const;

  std::string toString(std::string indent = "") const override;

protected:
  StructGetInst(llvm::CallInst &call_inst);
};

struct IndexGetInst : public GetInst {
public:
  Collection &getCollectionAccessed() const override;

  unsigned getNumberOfDimensions() const;
  llvm::Value &getIndexOperand(unsigned dim_idx) const;
  llvm::Use &getIndexOperandAsUse(unsigned dim_idx) const;

  std::string toString(std::string indent = "") const override;

protected:
  IndexGetInst(llvm::CallInst &call_inst);
};

struct AssocGetInst : public GetInst {
public:
  Collection &getCollectionAccessed() const override;

  llvm::Value &getKeyOperand() const;
  llvm::Use &getKeyOperandAsUse() const;

  std::string toString(std::string indent = "") const override;

protected:
  AssocGetInst(llvm::CallInst &call_inst);
};

/*
 * Collection operations
 */
struct DeleteStructInst : public MemOIRInst {
public:
  Struct &getStructDeleted() const;
  llvm::Value &getStructOperand() const;
  llvm::Use &getStructOperandAsUse() const;

  std::string toString(std::string indent = "") const override;

protected:
  DeleteStructInst(llvm::CallInst &call_inst);
};

struct DeleteCollectionInst : public MemOIRInst {
public:
  Collection &getCollectionDeleted() const;
  llvm::Value &getCollectionOperand() const;
  llvm::Use &getCollectionOperandAsUse() const;

  std::string toString(std::string indent = "") const override;

protected:
  DeleteCollectionInst(llvm::CallInst &call_inst);
};

struct JoinInst : public MemOIRInst {
public:
  Collection &getCollection() const;

  unsigned getNumberOfJoins() const;
  llvm::Value &getNumberOfJoinsOperand() const;
  llvm::Use &getNumberOfJoinsOperandAsUse() const;

  Collection &getCollectionJoined(unsigned join_index) const;
  llvm::Value &getJoinedOperand(unsigned join_index) const;
  llvm::Use &getJoinedOperandAsUse(unsigned join_index) const;

  std::string toString(std::string indent = "") const override;

protected:
  JoinInst(llvm::CallInst &call_inst);
};

struct SliceInst : public MemOIRInst {
public:
  Collection &getSlice() const;
  llvm::Value &getSlicedCollectionAsValue() const;

  Collection &getCollection() const;
  llvm::Value &getCollectionOperand() const;
  llvm::Use &getCollectionOperandAsUse() const;

  llvm::Value &getBeginIndex() const;
  llvm::Use &getBeginIndexAsUse() const;

  llvm::Value &getEndIndex() const;
  llvm::Use &getEndIndexAsUse() const;

  std::string toString(std::string indent = "") const override;

protected:
  SliceInst(llvm::CallInst &call_inst);
};

/*
 * Type checking
 */
struct AssertStructTypeInst : public MemOIRInst {
public:
  Type &getType() const;
  llvm::Value &getTypeOperand() const;
  llvm::Use &getTypeOperandAsUse() const;

  Struct &getStruct();
  llvm::Value &getStructOperand() const;
  llvm::Use &getStructOperandAsUse() const;

  std::string toString(std::string indent = "") const override;

protected:
  AssertStructTypeInst(llvm::CallInst &call_inst);
};

struct AssertCollectionTypeInst : public MemOIRInst {
public:
  Type &getType() const;
  llvm::Value &getTypeOperand() const;
  llvm::Use &getTypeOperandAsUse() const;

  Collection &getCollection() const;
  llvm::Value &getCollectionOperand() const;
  llvm::Use &getCollectionOperandAsUse() const;

  std::string toString(std::string indent = "") const override;

protected:
  AssertCollectionTypeInst(llvm::CallInst &call_inst);
};

struct ReturnTypeInst : public MemOIRInst {
public:
  Type &getType() const;
  llvm::Value &getTypeOperand() const;
  llvm::Use &getTypeOperandAsUse() const;

  std::string toString(std::string indent = "") const override;

protected:
  ReturnTypeInst(llvm::CallInst &call_inst);
};

} // namespace llvm::memoir

#endif
