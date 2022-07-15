#ifndef COMMON_ACCESSANALYSIS_H
#define COMMON_ACCESSANALYSIS_H
#pragma once

#include <unordered_map>
#include <unordered_set>

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"

#include "common/utility/FunctionNames.hpp"

#include "common/analysis/AllocationAnalysis.hpp"
#include "common/analysis/TypeAnalysis.hpp"

/*
 * This file provides a simple analysis interface to query information
 *   about an access to a MemOIR object.
 *
 * Author(s): Tommy McMichen
 * Created: July 12, 2022
 */

namespace llvm::memoir {

class AccessSummary;

/*
 * Access Analysis
 *
 * Top level entry for MemOIR access analysis.
 *
 * This access analysis provides points-to information that is:
 *
 * Field Sensitive: Treats each field of a struct separately.
 *   This information is made available in the FieldAccessSummary.
 *
 * Tensor In-sensitive: Treats each element of a tensor the same, but provides
 *   the index Value(s).
 *   This information is made available in the TensorAccessSummary.
 *
 * Flow In-sensitive: Does not provide additional information about the
 *   call-site and control flow that causes a may points-to access.
 *
 */
class AccessAnalysis {
public:
  /*
   * Singleton access
   */
  static AccessAnalysis &get(Module &M);

  /*
   * Top-level entry point
   */
  AccessSummary *getAccessSummary(llvm::CallInst &call_inst);

private:
  /*
   * Passed state
   */
  llvm::Module &M;

  /*
   * Memoized access summaries
   */
  std::unordered_map<llvm::CallInst *, AccessSummary *> access_summaries;

  /*
   * Internal helper functions
   */
  bool isRead(MemOIR_Func func_enum);
  bool isWrite(MemOIR_Func func_enum);

  /*
   * Constructor
   */
  AccessAnalysis(llvm::Module &M);

  /*
   * Singleton access protection
   * Do NOT implement these methods.
   */
  AccessAnalysis(AccessAnalysis const &);
  operator=(AccessAnalysis const &);

public:
  AccessAnalysis(AccessAnalysis const &) = delete;
  void operator=(AccessAnalysis const &) = delete;
};

/*
 * Field Summary
 *
 * Represents a field of a MemOIR object.
 */
class FieldSummary {
public:
  AllocationCode getCode();
  AllocationSummary &pointsTo();
  TypeSummary &getType();

private:
  AllocationSummary &points_to;
  TypeSummary &type;
  llvm::CallInst &call_inst;

  FieldSummary(llvm::CallInst &call_inst, AllocationSummary &points_to);
};

/*
 * Struct Field Summary
 *
 * Represents a field of a MemOIR struct.
 */
class StructFieldSummary {
public:
  uint64_t getIndex();

private:
  uint64_t index;

  StructFieldSummary(llvm::CallInst &call_inst,
                     AllocationSummary &points_to,
                     uint64_t index);

  friend class AccessAnalysis;
};

/*
 * Tensor Element Summary
 *
 * Represents an element of a MemOIR tensor.
 */
class TensorElementSummary : public FieldSummary {
  llvm::Value &getIndex(uint64_t dimension_index);
  uint64_t getNumberOfDimensions();

private:
  std::vector<llvm::Value *> indices;

  TensorElementSummary(llvm::CallInst &call_inst,
                       AllocationSummary &points_to,
                       uint64_t index);

  friend class AccessAnalysis;
};

/*
 * Points To Info
 * The lack of a points-to relation implies that an access NEVER points to the
 * given allocation.
 */
enum PointsToInfo {
  Must,
  May,
};

/*
 * Access Info
 * Describes the nature of the access.
 * NOTE: Currently an instruction is only able to either read OR write.
 */
enum AccessInfo {
  Read,
  Write,
};

/*
 * Access Summary
 *
 * Represents an access to a MemOIR object.
 * For every access we know:
 *   - The points to information, either may or must
 *   - The access information, either read or write
 *   - The type, as MemOIR is statically typed.
 */
class AccessSummary {
public:
  bool isMust();
  bool isMay();
  PointsToInfo getPointsToInfo();

  bool isRead();
  bool isWrite();
  AccessInfo getAccessInfo();

  virtual TypeSummary &getType() = 0;

private:
  PointsToInfo points_to_info;
  AccessInfo access_info;
  llvm::CallInst &call_inst;

protected:
  AccessSummary(llvm::CallInst &call_inst,
                PointsToInfo points_to_info,
                AccessInfo access_info);

  friend class AccessAnalysis;
};

/*
 * Read Summary
 *
 * Represents a read access to a MemOIR object.
 * A ReadSummary may be wrapped by a MayReadSummary if it is one possible Read
 */
class ReadSummary : public AccessSummary {
public:
  FieldSummary &getField();
  TypeSummary &getType() override;

private:
  FieldSummary &field;

protected:
  ReadSummary(llvm::CallInst &call_inst,
              PointsToInfo points_to_info,
              FieldSummary &field);

  friend class AccessAnalysis;
};

/*
 * Read Summary
 *
 * Represents a write access to a MemOIR object.
 * A ReadSummary may be wrapped by a MayReadSummary if it is one possible Read
 */
class WriteSummary : public AccessSummary {
public:
  llvm::Value &getValueWritten();
  FieldSummary &getField();
  TypeSummary &getType() override;

private:
  llvm::Value &value_written;
  FieldSummary &field;

protected:
  WriteSummary(llvm::CallInst &call_inst,
               PointsToInfo points_to_info,
               FieldSummary &field);

  friend class AccessAnalysis;
};

/*
 * May Read Access Summary
 *
 * Represents a read access to a MemOIR struct that may occur.
 */
class MayReadSummary : public AccessSummary {
public:
  typedef std::unordered_set<ReadSummary *>::const_iterator iterator;

  iterator begin();
  iterator end();

private:
  std::unordered_set<ReadSummary *> may_read_summaries;

protected:
  MayReadSummary(llvm::CallInst &call_inst,
                 std::unordered_set<ReadSummary *> &may_read_summaries);

  friend class AccessAnalysis;
};

/*
 * May Write Summary
 *
 * Represents a write access to MemOIR object that may occur.
 */
class MayWriteSummary {
public:
  typedef std::unordered_set<WriteSummary *>::const_iterator iterator;

  iterator begin();
  iterator end();

private:
  std::unordered_set<WriteSummary *> may_write_summaries;

protected:
  MayWriteSummary(llvm::CallInst &call_inst,
                  std::unordered_set<ReadSummary *> &may_write_summaries);

  friend class AccessAnalysis;
};

} // namespace llvm::memoir

#endif
