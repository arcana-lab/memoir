#ifndef COMMON_ACCESSANALYSIS_H
#define COMMON_ACCESSANALYSIS_H
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
  CollectionCode getCode() const;

  bool operator==(const CollectionSummary &other) const;

protected:
  CollectionCode code;

  CollectionSummary(CollectionCode code);

  friend class CollectionAnalysis;
};

class BaseCollectionSummary : public CollectionSummary {
public:
  AllocationSummary &getAllocation() const;

  bool operator==(const DefPHISummary &other) const;

protected:
  AllocationSummary &allocation;

  BaseCollectionSummary(AllocationSummary &allocation);

  friend class CollectionAnalysis;
};

class FieldArraySummary : public CollectionSummary {
public:
  static FieldArraySummary &get(TypeSummary &type, unsigned field_index);

  TypeSummary &getType() const;
  unsigned getIndex() const;

  bool operator==(const DefPHISummary &other) const;

protected:
  FieldArraySummary(TypeSummary &type, unsigned field_index);

  TypeSummary &type;
  unsigned field_index;

  /*
   * A mapping from (Struct Type, Field Index) to FieldArraySummary
   */
  static std::unordered_map<std::pair<TypeSummary *, unsigned>,
                            FieldArraySummary *>
      field_array_summaries

      friend class CollectionAnalysis;
}

class ControlPHISummary : public CollectionSummary {
public:
  CollectionSummary &getIncomingCollection(unsigned idx) const;
  CollectionSummary &getIncomingCollectionForBlock(
      const llvm::BasicBlock &BB) const;
  llvm::BasicBlock &getIncomingBlock(unsigned idx) const;
  unsigned getNumIncoming() const;
  llvm::PHINode &getPHI() const;

  bool operator==(const ControlPHISummary &other) const;

protected:
  llvm::PHINode &phi_node;
  std::unordered_map<llvm::BasicBlock *, CollectionSummary *> incoming

  ControlPHISummary(
      llvm::PHINode phi_node,
      std::unordered_map<llvm::BasicBlock *, CollectionSummary *> &incoming);

  friend class CollectionAnalysis;
};

class DefPHISummary : public CollectionSummary {
public:
  CollectionSummary &getCollection();
  AccessSummary &getAccess();

  bool operator==(const DefPHISummary &other) const;

protected:
  CollectionSummary &collection;
  AccessSummary &access;

  DefPHISummary(CollectionSummary &collection, AccessSummary &access);

  friend class CollectionAnalysis;
};

class UsePHISummary : public CollectionSummary {
public:
  CollectionSummary &getCollection();
  AccessSummary &getAccess();

  bool operator==(const UsePHISummary &other) const;

protected:
  CollectionSummary &collection;
  AccessSummary &access;

  UsePHISummary(CollectionSummary &collection, AccessSummary &access);

  friend class CollectionAnalysis;
};

} // namespace llvm::memoir

#endif
