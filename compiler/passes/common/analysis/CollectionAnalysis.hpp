#ifndef COMMON_COLLECTIONANALYSIS_H
#define COMMON_COLLECTIONANALYSIS_H
#pragma once

#include <iostream>

#include "common/support/InternalDatatypes.hpp"

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

#include "common/utility/FunctionNames.hpp"

#include "common/analysis/AccessAnalysis.hpp"
#include "common/analysis/AllocationAnalysis.hpp"
#include "common/analysis/TypeAnalysis.hpp"

/*
 * This file provides a simple analysis interface to query information
 *   about an MemOIR collections in a program.
 *
 * Author(s): Tommy McMichen
 * Created: November 14, 2022
 */

namespace llvm::memoir {

class TypeSummary;
class AccessSummary;
class AllocationSummary;
class CollectionAllocationSummary;
class CollectionSummary;

class CollectionAnalysis {
public:
  /*
   * Singleton access
   */
  static CollectionAnalysis &get(Module &M);

  static void invalidate(Module &M);

  /**
   * Get the CollectionSummary for a given memoir call.
   *
   * @param value A reference to an llvm Value
   * @return The collection summary, or NULL if value is not a memoir call.
   */
  CollectionSummary *getCollectionSummary(llvm::Value &value);

private:
  /*
   * Top-level drivers
   */
  void initialize();
  void analyze();

  /*
   * Analysis
   */

  /*
   * Private constructor and logistics.
   */
  CollectionAnalysis(llvm::Module &M);
  std::unordered_map<llvm::Module *, CollectionAnalysis *> analyses;

  void invalidate();
};

enum CollectionCode {
  BASE,
  FIELD_ARRAY,
  CONTROL_PHI,
  DEF_PHI,
  USE_PHI,
};

class CollectionSummary {
public:
  virtual TypeSummary &getElementType() const = 0;

  CollectionCode getCode() const;

  bool operator==(const CollectionSummary &other) const;
  friend std::ostream &operator<<(std::ostream &os,
                                  const AllocationSummary &as);
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                       const AllocationSummary &as);
  virtual std::string toString(std::string indent = "") const = 0;

protected:
  CollectionCode code;

  CollectionSummary(CollectionCode code);

  friend class CollectionAnalysis;
};

class BaseCollectionSummary : public CollectionSummary {
public:
  CollectionAllocationSummary &getAllocation() const;
  TypeSummary &getElementType() const override;

  bool operator==(const BaseCollectionSummary &other) const;
  std::string toString(std::string indent = "") const;

protected:
  CollectionAllocationSummary &allocation;

  BaseCollectionSummary(AllocationSummary &allocation);

  friend class CollectionAnalysis;
};

class FieldArraySummary : public CollectionSummary {
public:
  static FieldArraySummary &get(TypeSummary &struct_type, unsigned field_index);

  TypeSummary &getType() const;
  TypeSummary &getStructType() const;
  unsigned getIndex() const;

  TypeSummary &getElementType() const override;

  bool operator==(const FieldArraySummary &other) const;
  std::string toString(std::string indent = "") const;

protected:
  TypeSummary &type;
  TypeSummary &struct_type;
  unsigned field_index;

  FieldArraySummary(TypeSummary &field_type,
                    TypeSummary &struct_type,
                    unsigned field_index);

  friend class CollectionAnalysis;
};

class ControlPHISummary : public CollectionSummary {
public:
  CollectionSummary &getIncomingCollection(unsigned idx) const;
  CollectionSummary &getIncomingCollectionForBlock(
      const llvm::BasicBlock &BB) const;
  llvm::BasicBlock &getIncomingBlock(unsigned idx) const;
  unsigned getNumIncoming() const;
  llvm::PHINode &getPHI() const;

  TypeSummary &getElementType() const override;

  bool operator==(const ControlPHISummary &other) const;
  std::string toString(std::string indent = "") const;

protected:
  llvm::PHINode &phi_node;
  map<llvm::BasicBlock *, CollectionSummary *> incoming;

  ControlPHISummary(llvm::PHINode phi_node,
                    map<llvm::BasicBlock *, CollectionSummary *> &incoming);

  friend class CollectionAnalysis;
};

class DefPHISummary : public CollectionSummary {
public:
  CollectionSummary &getCollection() const;
  AccessSummary &getAccess() const;

  TypeSummary &getElementType() const override;

  bool operator==(const DefPHISummary &other) const;
  std::string toString(std::string indent = "") const;

protected:
  CollectionSummary &collection;
  AccessSummary &access;

  DefPHISummary(CollectionSummary &collection, AccessSummary &access);

  friend class CollectionAnalysis;
};

class UsePHISummary : public CollectionSummary {
public:
  CollectionSummary &getCollection() const;
  AccessSummary &getAccess() const;

  TypeSummary &getElementType() const override;

  bool operator==(const UsePHISummary &other) const;
  std::string toString(std::string indent = "") const;

protected:
  CollectionSummary &collection;
  AccessSummary &access;

  UsePHISummary(CollectionSummary &collection, AccessSummary &access);

  friend class CollectionAnalysis;
};

} // namespace llvm::memoir

#endif
