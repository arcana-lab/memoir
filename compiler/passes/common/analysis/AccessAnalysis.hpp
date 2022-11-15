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
class StructFieldSummary;
class TensorElementSummary;

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

  /**
   * Get the access summary for the given memoir call.
   *
   * @param value A reference to an llvm Value.
   * @return The access summary, or NULL if value is not a memoir call.
   */
  AccessSummary *getAccessSummary(llvm::Value &value);

  /**
   * Get the field summaries for a given llvm Value.
   *
   * @param value A reference to an llvm Value.
   * @return A reference to a set of field summaries that this value represents,
   *         if value is not a memoir field, this is an empty set.
   */
  set<FieldSummary *> &getFieldSummaries(llvm::Value &value);

  /**
   * Get the object summaries for a given llvm Value.
   *
   * @param value A reference to an llvm Value.
   * @return A reference to a set of object summaries that this value
   *         represents, if value is not a memoir field, this is an empty set.
   */
  set<ObjectSummary *> &getObjectSummaries(llvm::Value &value);

  /**
   * Get all accesses to a given field.
   *
   * @param field A reference to a memoir field summary.
   * @returns A reference to a set of access summaries.
   */
  set<AccessSummary *> &getFieldAccesses(FieldSummary &field);

  /**
   * Get all fields of a given object.
   *
   * @param object A reference to a memoir field summary.
   * @returns A reference to a set of access summaries.
   */
  set<FieldSummary *> &getObjectFields(ObjectSummary &object);

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
  map<AllocationSummary *, BaseObjectSummary *> base_object_summaries;
  map<FieldSummary *, ObjectSummary *> nested_object_summaries;
  map<llvm::CallInst *, map<FieldSummary *, ReferencedObjectSummary *>>
      reference_summaries;
  map<ObjectSummary *, set<FieldSummary *>> field_summaries;
  map<llvm::Value *, AccessSummary *> access_summaries;

  /*
   * Internal state
   */
  map<llvm::Value *, set<FieldSummary *>> value_to_field_summaries;
  map<llvm::Value *, set<ObjectSummary *>> value_to_object_summaries;
  map<FieldSummary *, set<AccessSummary *>> field_accesses;
  map<ReferencedObjectSummary *, set<FieldSummary *>> fields_to_reconcile;

  /*
   * Analysis driver functions
   */
  void initialize();
  void analyze();
  void analyzeObjects();
  void analyzeFields();
  void reconcileReferences();
  void analyzeAccesses();

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

  /*
   * Memoize and build functions
   */
  StructFieldSummary &fetchOrCreateStructFieldSummary(ObjectSummary &object,
                                                      uint64_t field_index);
  TensorElementSummary &fetchOrCreateTensorElementSummary(
      ObjectSummary &object,
      vector<llvm::Value *> &indices);

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

enum ObjectCode { BASE, NESTED, REFERENCED, DEF_PHI, USE_PHI };

/*
 * Object Summary
 *
 * Represents a dynamic instance of a MemOIR object.
 */
class ObjectSummary {
public:
  ObjectCode getCode() const;
  bool isNested() const;

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

  ObjectSummary(bool is_nested);

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
 * Nested Object Summary
 *
 * Represents a nested object within another MemOIR object.
 * This could be a struct within a struct, an element of a
 * tensor of structs, etc.
 */
class NestedObjectSummary : public ObjectSummary {
public:
  FieldSummary &getField() const;
  AllocationSummary &getAllocation() const override;
  TypeSummary &getType() const override;

  std::string toString(std::string indent = "") const override;

protected:
  FieldSummary &field;

  NestedObjectSummary(FieldSummary &field);

  friend class AccessAnalysis;
};

/*
 * Referenced Object Summary
 *
 * Represents an object or set of objects referenced by another MemOIR object.
 * This summary is flow-sensitive.
 */
class ReferencedObjectSummary : public ObjectSummary {
public:
  llvm::CallInst &getCallInst() const;
  FieldSummary &getField() const;
  AllocationSummary &getAllocation() const override;
  TypeSummary &getType() const override;

protected:
  llvm::CallInst &call_inst;
  FieldSummary &field;
  set<ObjectSummary *> referenced_objects;

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

  FieldSummary(ObjectSummary &points_to);
};

/*
 * Struct Field Summary
 *
 * Represents a field of a MemOIR struct.
 */
class StructFieldSummary : public FieldSummary {
  ublic : uint64_t getIndex() const;
  TypeSummary &getType() const;

  std::string toString(std::string indent = "") const override;

protected:
  uint64_t index;

  StructFieldSummary(ObjectSummary &points_to, uint64_t index);

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
  vector<llvm::Value *> indices;

  TensorElementSummary(ObjectSummary &points_to,
                       vector<llvm::Value *> &indices);

  friend class AccessAnalysis;
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

  bool isRead() const;
  bool isWrite() const;
  AccessInfo getAccessInfo() const;

  llvm::CallInst &getCallInst() const;
  TypeSummary &getType() const;

  /*
   * Returns the single field that MUST be read.
   * If more than one field may be read, return NULL.
   */
  FieldSummary *getSingleField() const;

  typedef set<FieldSummary *>::iterator iterator;
  typedef set<FieldSummary *>::const_iterator const_iterator;

  iterator begin();
  iterator end();
  const_iterator cbegin() const;
  const_iterator cend() const;

  friend bool operator<(const AccessSummary &l, const AccessSummary &r);
  friend std::ostream &operator<<(std::ostream &os, const AccessSummary &as);
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                       const AccessSummary &as);
  virtual std::string toString(std::string indent = "") const = 0;

private:
  llvm::CallInst &call_inst;
  TypeSummary &type;
  set<FieldSummary *> fields_accessed;

  AccessInfo access_info;

protected:
  AccessSummary(llvm::CallInst &call_inst,
                PointsToInfo points_to_info,
                AccessInfo access_info);

  friend class AccessAnalysis;
};

/*
 * Read Access Summary
 *
 * Represents a read access to a MemOIR field.
 */
class ReadSummary : public AccessSummary {
public:
  std::string toString(std::string indent = "") const override;

protected:
  ReadSummary(llvm::CallInst &call_inst, set<FieldSummary *> &fields_read);

  friend class AccessAnalysis;
};

/*
 * Write Access Summary
 *
 * Represents a write access to MemOIR field.
 */
class WriteSummary : public AccessSummary {
public:
  llvm::Value &getValueWritten() const;

  std::string toString(std::string indent = "") const override;

protected:
  llvm::Value &value_written;

  WriteSummary(llvm::CallInst &call_inst,
               set<FieldSummary *> &fields_written,
               llvm::Value &value_written);

  friend class AccessAnalysis;
};

} // namespace llvm::memoir

#endif
