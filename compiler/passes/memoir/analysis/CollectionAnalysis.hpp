#ifndef COMMON_COLLECTIONANALYSIS_H
#define COMMON_COLLECTIONANALYSIS_H
#pragma once

#include <iostream>

#include "common/support/Assert.hpp"
#include "common/support/InternalDatatypes.hpp"

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"

#include "common/ir/Collections.hpp"

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
  Collection *getCollectionSummary(llvm::Value &value);

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

} // namespace llvm::memoir

#endif
