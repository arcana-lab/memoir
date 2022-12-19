#ifndef COMMON_ALLOCATIONANALYSIS_H
#define COMMON_ALLOCATIONANALYSIS_H
#pragma once

#include <iostream>

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

#include "memoir/analysis/TypeAnalysis.hpp"
#include "memoir/support/InternalDatatypes.hpp"
#include "memoir/utility/FunctionNames.hpp"

/*
 * This file provides a simple analysis to quickly find information
 * relevant to a MemOIR allocation call.
 *
 * Author(s): Tommy McMichen
 * Created: July 11, 2022
 */

namespace llvm::memoir {

class AllocationSummary;

/*
 * Allocation Analysis
 *
 * Top level entry for MemOIR allocation analysis
 *
 * This allocation analysis provides basic information about a MemOIR
 *   allocation, it does not provide information about escapes and
 *   dynamic instances, but can easily be used for that.
 */
class AllocationAnalysis {
public:
  /*
   * Singleton access
   */
  static AllocationAnalysis &get(Module &M);
  static void invalidate(Module &M);

  /*
   * Top-level entry points.
   *
   * getAllocationSummaries
   *   Get all possible allocation summaries held by this LLVM value.
   *
   * getAllocationSummary
   *   Fet the allocation summary for the given MemOIR call.
   */
  set<AllocationSummary *> &getAllocationSummaries(llvm::Value &value);
  AllocationSummary *getAllocationSummary(llvm::CallInst &call_inst);

  /*
   * This class is not cloneable nor assignable.
   */
  AllocationAnalysis(AllocationAnalysis &other) = delete;
  void operator=(const AllocationAnalysis &) = delete;

private:
  /*
   * Passed state
   */
  llvm::Module &M;

  /*
   * Memoized allocation summaries
   */
  map<llvm::Value *, set<AllocationSummary *>> allocation_summaries;
  map<llvm::CallInst *, AllocationSummary *> the_allocation_summaries;

  /*
   * Internal helper functions
   */
  AllocationSummary *getStructAllocationSummary(llvm::CallInst &call_inst);
  AllocationSummary *getTensorAllocationSummary(llvm::CallInst &call_inst);
  AllocationSummary *getAssocArrayAllocationSummary(llvm::CallInst &call_inst);
  AllocationSummary *getSequenceAllocationSummary(llvm::CallInst &call_inst);
  void invalidate();

  /*
   * Constructor
   */
  AllocationAnalysis(llvm::Module &M);

  /*
   * Analyses
   */
  static map<llvm::Module *, AllocationAnalysis *> analyses;
};

} // namespace llvm::memoir

#endif
