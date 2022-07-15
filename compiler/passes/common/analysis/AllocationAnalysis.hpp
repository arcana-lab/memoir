#ifndef COMMON_ALLOCATIONANALYSIS_H
#define COMMON_ALLOCATIONANALYSIS_H
#pragma once

#include <unordered_map>

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"

#include "common/analysis/TypeAnalysis.hpp"
#include "common/utility/FunctionNames.hpp"

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

  /*
   * Top-level entry point.
   */
  AllocationSummary *getAllocationSummary(CallInst &call_inst);

  /*
   * This class is not cloneable nor assignable.
   */
  AllocationAnalysis(AllocationAnalysis const &) = delete;
  void operator=(AllocationAnalysis const &) = delete;

private:
  /*
   * Passed state
   */
  Module &M;

  /*
   * Memoized allocation summaries
   */
  std::unordered_map<CallInst *, AllocationSummary *> allocation_summaries;

  /*
   * Internal helper functions
   */
  TypeSummary *getTypeSummary(Value &V);

  /*
   * Constructor
   */
  AllocationAnalysis(Module &M);
};

/*
 * Allocation Code
 * Basic information about the class of object being allocated.
 */
enum AllocationCode { StructAlloc, TensorAlloc };

/*
 * Allocation Summary
 *
 * Represents an allocation of a MemOIR object.
 * For every allocation we know:
 *   - The statically know type of the allocation
 */
struct AllocationSummary {
public:
  TypeSummary &getTypeSummary();
  AllocationCode getCode();
  CallInst &getCallInst();

  virtual std::string toString() = 0;

private:
  AllocationCode code;
  TypeSummary &type_summary;
  CallInst &call_inst;

  AllocationSummary(AllocationCode code, CallInst &call_inst);
};

/*
 * Struct Allocation Summary
 *
 * Represents an allocation of a MemOIR struct.
 */
struct StructAllocationSummary : public AllocationSummary {
public:
  static AllocationSummary &get(CallInst &call_inst);

  std::string toString();

private:
  StructAllocationSummary(CallInst &call_inst);

  friend class AllocationAnalysis;
};

/*
 * Tensor Allocation Summary
 *
 * Represents an allocation of a MemOIR tensor
 */
struct TensorAllocationSummary : public AllocationSummary {
public:
  static AllocationSummary &get(
      CallInst &call_inst,
      std::vector<llvm::Value *> &length_of_dimensions);

  TypeSummary &element_type_summary;
  std::vector<llvm::Value *> length_of_dimensions;

  std::string toString();

private:
  TensorAllocationSummary(CallInst &call_inst,
                          std::vector<llvm::Value *> &length_of_dimensions);

  friend class AllocationAnalysis;
};

} // namespace llvm::memoir

#endif
