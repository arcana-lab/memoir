#ifndef COMMON_ALLOCATIONANALYSIS_H
#define COMMON_ALLOCATIONANALYSIS_H
#pragma once

#include <iostream>

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

#include "common/analysis/TypeAnalysis.hpp"
#include "common/support/InternalDatatypes.hpp"
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

/*
 * Allocation Code
 * Basic information about the class of object being allocated.
 */
enum AllocationCode { STRUCT, TENSOR, ASSOC_ARRAY, SEQUENCE };

/*
 * Allocation Summary
 *
 * Represents an allocation of a MemOIR object.
 * For every allocation we know:
 *   - The statically know type of the allocation
 */
struct AllocationSummary {
public:
  TypeSummary &getType() const;
  AllocationCode getCode() const;
  llvm::CallInst &getCallInst() const;

  friend std::ostream &operator<<(std::ostream &os,
                                  const AllocationSummary &as);
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                       const AllocationSummary &as);
  virtual std::string toString(std::string indent = "") const = 0;

protected:
  AllocationCode code;
  TypeSummary &type;
  llvm::CallInst &call_inst;

  AllocationSummary(llvm::CallInst &call_inst,
                    AllocationCode code,
                    TypeSummary &type);
};

/*
 * Struct Allocation Summary
 *
 * Represents an allocation of a MemOIR struct.
 */
struct StructAllocationSummary : public AllocationSummary {
public:
  std::string toString(std::string indent = "") const override;

protected:
  StructAllocationSummary(llvm::CallInst &call_inst, TypeSummary &type);

  friend class AllocationAnalysis;
};

/*
 * Tensor Allocation Summary
 *
 * Represents an allocation of a MemOIR tensor
 */
struct TensorAllocationSummary : public AllocationSummary {
public:
  TypeSummary &getElementType() const;
  uint64_t getNumberOfDimensions() const;
  llvm::Value *getLengthOfDimension(uint64_t dimension_index) const;

  std::string toString(std::string indent = "") const override;

protected:
  TypeSummary &element_type;
  std::vector<llvm::Value *> length_of_dimensions;

  TensorAllocationSummary(llvm::CallInst &call_inst,
                          TypeSummary &element_type,
                          std::vector<llvm::Value *> &length_of_dimensions);

  friend class AllocationAnalysis;
};

/*
 * Associative Array Allocation Summary
 *
 * Represents an allocation of a MemOIR associative array.
 */
struct AssocArrayAllocationSummary : public AllocationSummary {
public:
  TypeSummary &getKeyType() const;
  TypeSummary &getValueType() const;

  std::string toString(std::string indent = "") const override;

protected:
  TypeSummary &key_type;
  TypeSummary &value_type;

  AssocArrayAllocationSummary(llvm::CallInst &call_inst,
                              TypeSummary &key_type,
                              TypeSummary &value_type);

  friend class AllocationAnalysis;
};

/*
 * Sequence Allocation Summary
 *
 * Represents an allocation of a MemOIR associative array.
 */
struct SequenceAllocationSummary : public AllocationSummary {
public:
  TypeSummary &getElementType() const;

  std::string toString(std::string indent = "") const override;

protected:
  TypeSummary &element_type;

  SequenceAllocationSummary(llvm::CallInst &call_inst,
                            TypeSummary &element_type);

  friend class AllocationAnalysis;
};

} // namespace llvm::memoir

#endif
