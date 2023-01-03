#ifndef COMMON_COLLECTIONS_H
#define COMMON_COLLECTIONS_H
#pragma once

#include "llvm/IR/Argument.h"

#include "memoir/support/Assert.hpp"
#include "memoir/support/InternalDatatypes.hpp"

#include "memoir/ir/Instructions.hpp"
#include "memoir/ir/Types.hpp"

namespace llvm::memoir {

enum CollectionCode {
  BASE,
  FIELD_ARRAY,
  NESTED,
  CONTROL_PHI,
  CALL_PHI,
  DEF_PHI,
  USE_PHI,
  JOIN_PHI,
};

struct CollectionAllocInst;
struct ReadInst;
struct WriteInst;
struct GetInst;
struct JoinInst;

struct Collection {
public:
  virtual CollectionType &getType() const = 0;
  virtual Type &getElementType() const = 0;

  CollectionCode getCode() const;

  bool operator==(const Collection &other) const;
  friend std::ostream &operator<<(std::ostream &os, const Collection &as);
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                       const Collection &as);
  virtual std::string toString(std::string indent = "") const = 0;

protected:
  CollectionCode code;

  Collection(CollectionCode code);

  friend class CollectionAnalysis;
};

struct BaseCollection : public Collection {
public:
  CollectionAllocInst &getAllocation() const;

  CollectionType &getType() const override;
  Type &getElementType() const override;

  bool operator==(const BaseCollection &other) const;
  std::string toString(std::string indent = "") const;

protected:
  CollectionAllocInst &allocation;

  BaseCollection(CollectionAllocInst &allocation);

  friend class CollectionAnalysis;
};

struct FieldArray : public Collection {
public:
  static FieldArray &get(StructType &struct_type, unsigned field_index);
  static FieldArray &get(FieldArrayType &field_array_type);

  StructType &getStructType() const;
  unsigned getFieldIndex() const;

  CollectionType &getType() const override;
  Type &getElementType() const override;

  bool operator==(const FieldArray &other) const;
  std::string toString(std::string indent = "") const;

protected:
  FieldArrayType &type;

  FieldArray(FieldArrayType &type);

  static map<FieldArrayType *, FieldArray *> field_array_type_to_field_array;

  friend class CollectionAnalysis;
};

struct NestedCollection : public Collection {
public:
  GetInst &getAccess() const;
  Collection &getNestingCollection() const;

  CollectionType &getType() const override;
  Type &getElementType() const override;

protected:
  GetInst &get_inst;

  NestedCollection(GetInst &get_inst);

  friend class NestedCollection;
};

struct ControlPHICollection : public Collection {
public:
  Collection &getIncomingCollection(unsigned idx) const;
  Collection &getIncomingCollectionForBlock(const llvm::BasicBlock &BB) const;
  llvm::BasicBlock &getIncomingBlock(unsigned idx) const;
  unsigned getNumIncoming() const;
  llvm::PHINode &getPHI() const;

  CollectionType &getType() const override;
  Type &getElementType() const override;

  bool operator==(const ControlPHICollection &other) const;
  std::string toString(std::string indent = "") const;

protected:
  llvm::PHINode &phi_node;
  map<llvm::BasicBlock *, Collection *> incoming;

  ControlPHICollection(llvm::PHINode phi_node,
                       map<llvm::BasicBlock *, Collection *> &incoming);

  friend class CollectionAnalysis;
};

struct CallPHICollection : public Collection {
public:
  Collection &getIncomingCollection(unsigned idx) const;
  Collection &getIncomingCollectionForCall(const llvm::BasicBlock &BB) const;
  llvm::CallBase &getIncomingCall(unsigned idx) const;
  unsigned getNumIncoming() const;
  llvm::Argument &getArgument() const;

  CollectionType &getType() const override;
  Type &getElementType() const override;

  bool operator==(const ControlPHICollection &other) const;
  std::string toString(std::string indent = "") const;

protected:
  llvm::Argument &argument;
  vector<llvm::CallBase *> incoming_calls;
  map<llvm::CallBase *, Collection *> incoming;

  CallPHICollection(llvm::Argument &argument,
                    map<llvm::CallBase *, Collection *> &incoming);

  friend class CollectionAnalysis;
};

struct DefPHICollection : public Collection {
public:
  Collection &getCollection() const;
  WriteInst &getAccess() const;

  CollectionType &getType() const override;
  Type &getElementType() const override;

  bool operator==(const DefPHICollection &other) const;
  std::string toString(std::string indent = "") const;

protected:
  WriteInst &access;

  DefPHICollection(Collection &collection, WriteInst &access);

  friend class CollectionAnalysis;
};

struct UsePHICollection : public Collection {
public:
  Collection &getCollection() const;
  ReadInst &getAccess() const;

  CollectionType &getType() const override;
  Type &getElementType() const override;

  bool operator==(const UsePHICollection &other) const;
  std::string toString(std::string indent = "") const;

protected:
  ReadInst &read;

  UsePHICollection(Collection &collection, ReadInst &read);

  friend class CollectionAnalysis;
};

struct JoinPHICollection : public Collection {
public:
  JoinInst &getJoin() const;
  unsigned getNumberOfJoinedCollections() const;
  Collection &getJoinedCollection(unsigned join_index) const;

  CollectionType &getType() const override;
  Type &getElementType() const override;

protected:
  JoinInst &join_inst;

  JoinPHICollection(JoinInst &join_inst);

  friend class CollectionAnalysis;
};

} // namespace llvm::memoir

#endif
