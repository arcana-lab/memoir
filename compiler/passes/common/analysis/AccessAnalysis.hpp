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

/*
 * Stub types needed by the AccessAnalysis
 */
class AccessSummary;
class ObjectSummary;
class BaseObjectSummary;
class FieldSummary;

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
  static void invalidate(Module &M);

  /*
   * Top-level entry point
   *
   * getAccessSummary
   *   Get the access summary for the given MemOIR call.
   *
   * getFieldSummaries
   *   Get all possible fields held by this LLVM Value.
   */
  AccessSummary *getAccessSummary(llvm::Value &value);
  set<FieldSummary *> &getFieldSummaries(llvm::Value &value);
  set<AccessSummary *> &getFieldAccesses(FieldSummary &field);

  /*
   * This class is not cloneable nor assignable.
   */
  AccessAnalysis(AccessAnalysis &other) = delete;
  void operator=(const AccessAnalysis &) = delete;

private:
  /*
   * Passed state
   */
  llvm::Module &M;

  /*
   * Owned state
   */
  map<llvm::Value *, AccessSummary *> access_summaries;
  map<llvm::Value *, set<FieldSummary *>> field_summaries;
  map<AllocationSummary *, BaseObjectSummary *> base_object_summaries;
  map<llvm::CallInst *, set<ObjectSummary *>> nested_object_summaries;

  /*
   * Internal state
   */
  map<llvm::Value *, set<ObjectSummary *>> object_summaries;
  map<FieldSummary *, set<AccessSummary *>> field_accesses;

  /*
   * Internal helper functions
   */
  AccessSummary *getAccessSummaryForCall(llvm::CallInst &call_inst);
  set<FieldSummary *> &getFieldSummariesForCall(llvm::CallInst &call_inst);
  set<FieldSummary *> &getStructFieldSummaries(llvm::CallInst &call_inst);
  set<FieldSummary *> &getTensorElementSummaries(llvm::CallInst &call_inst);
  set<ObjectSummary *> &getReadStructSummaries(llvm::CallInst &call_inst);
  set<ObjectSummary *> &getReadTensorSummaries(llvm::CallInst &call_inst);
  set<ObjectSummary *> &getReadReferenceSummaries(llvm::CallInst &call_inst);
  set<ObjectSummary *> &getObjectSummaries(llvm::Value &value);

  static bool isRead(MemOIR_Func func_enum);
  static bool isWrite(MemOIR_Func func_enum);
  static bool isAccess(MemOIR_Func func_enum);

  void invalidate();

  /*
   * Constructor
   */
  AccessAnalysis(llvm::Module &M);

  /*
   * Factory
   */
  static map<llvm::Module *, AccessAnalysis *> analyses;
};

enum ObjectCode { BASE, NESTED_STRUCT, NESTED_TENSOR };

/*
 * Object Summary
 *
 * Represents a dynamic instance of a MemOIR object.
 */
class ObjectSummary {
public:
  bool isNested() const;
  ObjectCode getCode() const;
  llvm::CallInst &getCallInst() const;

  virtual AllocationSummary &getAllocation() const = 0;
  virtual TypeSummary &getType() const = 0;

  AllocationCode getAllocationCode() const;
  TypeCode getTypeCode() const;

  virtual std::string toString(std::string indent = "") const = 0;
  friend std::ostream &operator<<(std::ostream &os,
                                  const ObjectSummary &summary);
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                       const ObjectSummary &summary);

protected:
  ObjectCode code;
  llvm::CallInst &call_inst;

  ObjectSummary(llvm::CallInst &call_inst, ObjectCode code = ObjectCode::BASE);

  friend class AccessAnalysis;
};

/*
 * Base Object Summary
 *
 * Represents a base allocation of a MemOIR object.
 */
class BaseObjectSummary : public ObjectSummary {
public:
  AllocationSummary &getAllocation() const override;
  TypeSummary &getType() const override;

  std::string toString(std::string indent = "") const override;

protected:
  AllocationSummary &allocation;

  BaseObjectSummary(AllocationSummary &allocation);

  friend class AccessAnalysis;
};

/*
 * Nested Struct Summary
 *
 * Represents a nested struct within another MemOIR object.
 * This could be a struct within a struct, an element of a
 * tensor of structs, etc.
 */
class NestedStructSummary : public ObjectSummary {
public:
  FieldSummary &getField() const;
  AllocationSummary &getAllocation() const override;
  TypeSummary &getType() const override;

  std::string toString(std::string indent = "") const override;

protected:
  FieldSummary &field;

  NestedStructSummary(llvm::CallInst &call_inst, FieldSummary &field);

  friend class AccessAnalysis;
};

/*
 * Nested Tensor Summary
 *
 * Represents a nested tensor within another MemOIR object.
 * This could be a tensor within a struct, an element of a
 * tensor of tensors, etc.
 *
 * NOTE: Nested Tensors must have a static length, otherwise it cannot be
 * lowered to contiguous memory.
 */
class NestedTensorSummary : public ObjectSummary {
public:
  FieldSummary &getField() const;
  AllocationSummary &getAllocation() const override;
  TypeSummary &getType() const override;

  std::string toString(std::string indent = "") const override;

protected:
  FieldSummary &field;

  NestedTensorSummary(llvm::CallInst &call_inst, FieldSummary &field);

  friend class AccessAnalysis;
};

/*
 * Field Summary
 *
 * Represents a field of a MemOIR object.
 */
class FieldSummary {
public:
  ObjectSummary &pointsTo() const;
  AllocationSummary &getAllocation() const;
  llvm::CallInst &getCallInst() const;
  TypeCode getTypeCode() const;

  virtual TypeSummary &getType() const = 0;

  virtual std::string toString(std::string indent = "") const = 0;
  friend std::ostream &operator<<(std::ostream &os,
                                  const FieldSummary &summary);
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                       const FieldSummary &summary);

protected:
  ObjectSummary &points_to;
  llvm::CallInst &call_inst;

  FieldSummary(llvm::CallInst &call_inst, ObjectSummary &points_to);
};

/*
 * Struct Field Summary
 *
 * Represents a field of a MemOIR struct.
 */
class StructFieldSummary : public FieldSummary {
public:
  uint64_t getIndex() const;
  TypeSummary &getType() const;

  std::string toString(std::string indent = "") const override;

protected:
  uint64_t index;

  StructFieldSummary(llvm::CallInst &call_inst,
                     ObjectSummary &points_to,
                     uint64_t index);

  friend class AccessAnalysis;
};

/*
 * Tensor Element Summary
 *
 * Represents an element of a MemOIR tensor.
 */
class TensorElementSummary : public FieldSummary {
public:
  llvm::Value &getIndex(uint64_t dimension_index) const;
  uint64_t getNumberOfDimensions() const;
  TypeSummary &getType() const;

  std::string toString(std::string indent = "") const override;

protected:
  std::vector<llvm::Value *> indices;

  TensorElementSummary(llvm::CallInst &call_inst,
                       ObjectSummary &points_to,
                       std::vector<llvm::Value *> &indices);

  friend class AccessAnalysis;
};

/*
 * Points To Info
 * The lack of a points-to relation implies that an access NEVER points to the
 * given allocation.
 */
enum PointsToInfo {
  MUST,
  MAY,
};

/*
 * Access Info
 * Describes the nature of the access.
 * NOTE: Currently an instruction is only able to either read OR write.
 */
enum AccessInfo {
  READ,
  WRITE,
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
  bool isMust() const;
  bool isMay() const;
  PointsToInfo getPointsToInfo() const;

  bool isRead() const;
  bool isWrite() const;
  AccessInfo getAccessInfo() const;

  llvm::CallInst &getCallInst() const;
  virtual TypeSummary &getType() const = 0;

  friend bool operator<(const AccessSummary &l, const AccessSummary &r);
  friend std::ostream &operator<<(std::ostream &os, const AccessSummary &as);
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                       const AccessSummary &as);
  virtual std::string toString(std::string indent = "") const = 0;

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
 * Must Read Access Summary
 *
 * Represents a read access to a MemOIR field that must occur.
 */
class MustReadSummary : public AccessSummary {
public:
  FieldSummary &getField() const;
  TypeSummary &getType() const override;

  std::string toString(std::string indent = "") const override;

protected:
  FieldSummary &field;

  MustReadSummary(llvm::CallInst &call_inst, FieldSummary &field);

  friend class AccessAnalysis;
};

/*
 * Must Write Summary
 *
 * Represents a write access to a MemOIR field that must occur.
 */
class MustWriteSummary : public AccessSummary {
public:
  TypeSummary &getType() const override;
  FieldSummary &getField() const;
  llvm::Value &getValueWritten() const;

  std::string toString(std::string indent = "") const override;

protected:
  FieldSummary &field;
  llvm::Value &value_written;

  MustWriteSummary(llvm::CallInst &call_inst,
                   FieldSummary &field,
                   llvm::Value &value_written);

  friend class AccessAnalysis;
};

/*
 * May Read Access Summary
 *
 * Represents a read access to a MemOIR field that may occur.
 */
class MayReadSummary : public AccessSummary {
public:
  typedef set<FieldSummary *>::iterator iterator;
  typedef set<FieldSummary *>::const_iterator const_iterator;

  iterator begin();
  iterator end();
  const_iterator cbegin() const;
  const_iterator cend() const;

  TypeSummary &getType() const override;

  std::string toString(std::string indent = "") const override;

protected:
  TypeSummary &type;
  set<FieldSummary *> may_read_summaries;

  MayReadSummary(llvm::CallInst &call_inst,
                 TypeSummary &type_summary,
                 set<FieldSummary *> &may_read_summaries);

  friend class AccessAnalysis;
};

/*
 * May Write Summary
 *
 * Represents a write access to MemOIR field that may occur.
 */
class MayWriteSummary : public AccessSummary {
public:
  typedef set<FieldSummary *>::iterator iterator;
  typedef set<FieldSummary *>::const_iterator const_iterator;

  iterator begin();
  iterator end();

  const_iterator cbegin() const;
  const_iterator cend() const;

  TypeSummary &getType() const override;
  llvm::Value &getValueWritten() const;

  std::string toString(std::string indent = "") const override;

protected:
  TypeSummary &type;
  set<FieldSummary *> may_write_summaries;
  llvm::Value &value_written;

  MayWriteSummary(llvm::CallInst &call_inst,
                  TypeSummary &type_summary,
                  set<FieldSummary *> &may_write_summaries,
                  llvm::Value &value_written);

  friend class AccessAnalysis;
};

} // namespace llvm::memoir

#endif
