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

class AllocationAnalysis {
public:
  static AllocationAnalysis &get(Module &M);

  AllocationSummary *getAllocationSummary(CallInst &call_inst);

private:
  Module &M;

  std::unordered_map<CallInst *, AllocationSummary *> allocation_summaries;

  TypeSummary *getTypeSummary(Value &V);

  AllocationAnalysis(Module &M);
};

enum AllocationCode { StructAlloc, TensorAlloc };

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

struct StructAllocationSummary : public AllocationSummary {
public:
  static AllocationSummary &get(CallInst &call_inst);

  std::string toString();

private:
  StructAllocationSummary(CallInst &call_inst);

  friend class AllocationAnalysis;
};

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
