#ifndef COMMON_COLLECTIONANALYSIS_H
#define COMMON_COLLECTIONANALYSIS_H
#pragma once

#include <iostream>

#include "memoir/support/Assert.hpp"
#include "memoir/support/InternalDatatypes.hpp"

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"

#include "memoir/ir/Collections.hpp"

#include "memoir/analysis/TypeAnalysis.hpp"

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
  static CollectionAnalysis &get();

  static Collection *analyze(llvm::Use &U);

  static Collection *analyze(llvm::Value &V);

  static void invalidate();

  /**
   * Get the Collection for a given llvm Use.
   *
   * @param use A reference to an llvm Use.
   * @return The collection, or NULL if the Use is not a memoir collection.
   */
  Collection *getCollection(llvm::Use &use);

  /**
   * Get the Collection for a given llvm Value.
   *
   * @param value A reference to an llvm Value.
   * @return The collection, or NULL if the Value is not a memoir collection.
   */
  Collection *getCollection(llvm::Value &value);

private:
  /*
   * Analysis
   */

  /*
   * Private constructor and logistics.
   */
  CollectionAnalysis();

  void _invalidate();
};

} // namespace llvm::memoir

#endif
