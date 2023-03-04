#ifndef COMMON_COLLECTIONS_H
#define COMMON_COLLECTIONS_H
#pragma once

#include "llvm/IR/Argument.h"

#include "memoir/support/Assert.hpp"
#include "memoir/support/InternalDatatypes.hpp"

#include "memoir/ir/Instructions.hpp"
#include "memoir/ir/Types.hpp"

namespace llvm::memoir {

struct CollectionAllocInst;
struct ReadInst;
struct WriteInst;
struct GetInst;
struct JoinInst;
struct SliceInst;

struct Type;
struct CollectionType;
struct FieldArrayType;
struct ReferenceType;

enum class CollectionKind {
  BASE,
  FIELD_ARRAY,
  NESTED,
  REFERENCED,
  CONTROL_PHI,
  CALL_PHI,
  ARG_PHI,
  RET_PHI,
  DEF_PHI,
  USE_PHI,
  JOIN_PHI,
  SLICE,
};

struct Collection {
private:
  const CollectionKind code;

public:
  CollectionKind getKind() const;

  virtual CollectionType &getType() const = 0;
  virtual Type &getElementType() const = 0;

  bool operator==(const Collection &other) const;
  friend std::ostream &operator<<(std::ostream &os, const Collection &as);
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                       const Collection &as);
  virtual std::string toString(std::string indent = "") const = 0;

protected:
  Collection(CollectionKind code);

  friend class CollectionAnalysis;
};

struct BaseCollection : public Collection {
public:
  CollectionAllocInst &getAllocation() const;

  CollectionType &getType() const override;
  Type &getElementType() const override;

  static bool classof(Collection *C) {
    return (C->getKind() == CollectionKind::BASE);
  };

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

  static bool classof(Collection *C) {
    return (C->getKind() == CollectionKind::FIELD_ARRAY);
  };

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

  static bool classof(Collection *C) {
    return (C->getKind() == CollectionKind::NESTED);
  };

  bool operator==(const NestedCollection &other) const;
  std::string toString(std::string indent = "") const;

protected:
  GetInst &access;

  NestedCollection(GetInst &get_inst);

  friend class CollectionAnalysis;
};

struct ReferencedCollection : public Collection {
public:
  ReadInst &getAccess() const;
  ReferenceType &getReferenceType() const;
  CollectionType &getType() const override;
  Type &getElementType() const override;

  static bool classof(Collection *C) {
    return (C->getKind() == CollectionKind::REFERENCED);
  };

  bool operator==(const ReferencedCollection &other) const;
  std::string toString(std::string indent = "") const;

protected:
  ReadInst &access;

  ReferencedCollection(ReadInst &read_inst);

  friend class CollectionAnalysis;
};

struct ControlPHICollection : public Collection {
public:
  Collection &getIncomingCollection(unsigned idx) const;
  Collection &getIncomingCollectionForBlock(llvm::BasicBlock &BB) const;
  llvm::BasicBlock &getIncomingBlock(unsigned idx) const;
  unsigned getNumIncoming() const;
  llvm::PHINode &getPHI() const;

  CollectionType &getType() const override;
  Type &getElementType() const override;

  static bool classof(Collection *C) {
    return (C->getKind() == CollectionKind::CONTROL_PHI);
  };

  bool operator==(const ControlPHICollection &other) const;
  std::string toString(std::string indent = "") const;

protected:
  llvm::PHINode &phi_node;
  map<llvm::BasicBlock *, Collection *> incoming;

  ControlPHICollection(llvm::PHINode &phi_node,
                       map<llvm::BasicBlock *, Collection *> &incoming);

  friend class CollectionAnalysis;
};

struct RetPHICollection : public Collection {
public:
  Collection &getIncomingCollection(unsigned idx) const;
  Collection &getIncomingCollectionForReturn(llvm::ReturnInst &I) const;
  llvm::ReturnInst &getIncomingReturn(unsigned idx) const;
  unsigned getNumIncoming() const;
  llvm::CallBase &getCall() const;

  CollectionType &getType() const override;
  Type &getElementType() const override;

  static bool classof(Collection *C) {
    return (C->getKind() == CollectionKind::RET_PHI);
  };

  bool operator==(const RetPHICollection &other) const;
  std::string toString(std::string indent = "") const;

protected:
  llvm::CallBase &call;
  vector<llvm::ReturnInst *> incoming_returns;
  map<llvm::ReturnInst *, Collection *> incoming;

  RetPHICollection(llvm::CallBase &call,
                   map<llvm::ReturnInst *, Collection *> &incoming);

  friend class CollectionAnalysis;
};

struct ArgPHICollection : public Collection {
public:
  Collection &getIncomingCollection(unsigned idx) const;
  Collection &getIncomingCollectionForCall(llvm::CallBase &CB) const;
  llvm::CallBase &getIncomingCall(unsigned idx) const;
  unsigned getNumIncoming() const;
  llvm::Argument &getArgument() const;

  CollectionType &getType() const override;
  Type &getElementType() const override;

  static bool classof(Collection *C) {
    return (C->getKind() == CollectionKind::ARG_PHI);
  };

  bool operator==(const ArgPHICollection &other) const;
  std::string toString(std::string indent = "") const;

protected:
  llvm::Argument &argument;
  vector<llvm::CallBase *> incoming_calls;
  map<llvm::CallBase *, Collection *> incoming;

  ArgPHICollection(llvm::Argument &argument,
                   map<llvm::CallBase *, Collection *> &incoming);

  friend class CollectionAnalysis;
};

struct DefPHICollection : public Collection {
public:
  Collection &getCollection() const;
  WriteInst &getAccess() const;

  CollectionType &getType() const override;
  Type &getElementType() const override;

  static bool classof(Collection *C) {
    return (C->getKind() == CollectionKind::DEF_PHI);
  };

  bool operator==(const DefPHICollection &other) const;
  std::string toString(std::string indent = "") const;

protected:
  WriteInst &access;

  DefPHICollection(WriteInst &access);

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
  ReadInst &access;

  UsePHICollection(ReadInst &read);

  friend class CollectionAnalysis;
};

struct JoinPHICollection : public Collection {
public:
  JoinInst &getJoin() const;
  unsigned getNumberOfJoinedCollections() const;
  Collection &getJoinedCollection(unsigned join_index) const;

  CollectionType &getType() const override;
  Type &getElementType() const override;

  static bool classof(Collection *C) {
    return (C->getKind() == CollectionKind::JOIN_PHI);
  };

  bool operator==(const JoinPHICollection &other) const;
  std::string toString(std::string indent = "") const;

protected:
  JoinInst &join_inst;

  JoinPHICollection(JoinInst &join_inst);

  friend class CollectionAnalysis;
};

struct SliceCollection : public Collection {
public:
  SliceInst &getSlice() const;
  Collection &getSlicedCollection() const;

  CollectionType &getType() const override;
  Type &getElementType() const override;

  static bool classof(Collection *C) {
    return (C->getKind() == CollectionKind::SLICE);
  };

  bool operator==(const SliceCollection &other) const;
  std::string toString(std::string indent = "") const;

protected:
  SliceInst &slice_inst;

  SliceCollection(SliceInst &slice_inst);

  friend class CollectionAnalysis;
};

} // namespace llvm::memoir

#endif
