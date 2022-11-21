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
#include "common/analysis/CollectionAnalysis.hpp"
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
class ReadSummary;
class WriteSummary;

class StructSummary;
class ReferencedStructSummary;

class CollectionSummary;
class FieldArraySummary;

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
   * Get the struct summary for a given llvm Value.
   *
   * @param value A reference to an llvm Value.
   * @return The struct summary, or NULL if value is not a memoir struct.
   */
  StructSummary *getStructSummary(llvm::Value &value);

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
  // map<AllocationSummary *, BaseObjectSummary *> base_object_summaries;
  // map<FieldArraySummary *, ObjectSummary *> nested_object_summaries;
  // map<AccessSummary *, ReferencedStructSummary *> reference_summaries;
  map<llvm::Value *, AccessSummary *> access_summaries;

  /*
   * Borrowed state
   */
  // map<llvm::Value *, StructSummary *> value_to_object_summaries;
  // map<FieldArraySummary *, set<AccessSummary *>> field_accesses;

  /*
   * Analysis driver functions
   */
  void initialize();
  void analyze();
  void analyzeStruct();
  void analyzeAccesses();

  /*
   * Internal helper functions
   */
  AccessSummary *getAccessSummaryForCall(llvm::CallInst &call_inst);
  StructSummary &getReadStructSummaries(llvm::CallInst &call_inst);
  ReferencedStructSummary &getReadReferenceSummaries(llvm::CallInst &call_inst);

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

class ReadSummary;
class WriteSummary;

enum class StructCode {
  BASE,
  NESTED,
  CONTAINED,
  REFERENCED,
  CONTROL_PHI,
  CALL_PHI
};

/*
 * Struct Summary
 *
 * Represents a dynamic instance of a MemOIR struct.
 */
class StructSummary {
public:
  StructCode getCode() const;

  virtual StructTypeSummary &getType() const = 0;

  virtual std::string toString(std::string indent = "") const = 0;
  friend std::ostream &operator<<(std::ostream &os,
                                  const StructSummary &summary);
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                       const StructSummary &summary);

protected:
  StructCode code;

  StructSummary(StructCode code);

  friend class AccessAnalysis;
};

/*
 * Base Struct Summary
 *
 * Represents a base allocation of a MemOIR object.
 */
class BaseStructSummary : public StructSummary {
public:
  StructAllocationSummary &getAllocation() const;
  StructTypeSummary &getType() const override;

  std::string toString(std::string indent = "") const override;

protected:
  StructAllocationSummary &allocation;

  BaseStructSummary(StructAllocationSummary &allocation);

  friend class AccessAnalysis;
};

/*
 * Nested Struct Summary
 *
 * Represents a nested struct within another MemOIR struct.
 * This could be a struct within a struct.
 */
class NestedStructSummary : public StructSummary {
public:
  FieldArraySummary &getFieldArray() const;
  StructSummary &getContainer() const;
  llvm::CallInst &getCallInst() const;
  StructTypeSummary &getType() const override;

  std::string toString(std::string indent = "") const override;

protected:
  FieldArraySummary &struct_container;

  NestedStructSummary(FieldArraySummary &field);

  friend class AccessAnalysis;
};

/*
 * Contained Struct Summary
 *
 * Represents a struct contained within a MemOIR collection.
 * This could be an element of a tensor of structs, etc.
 */
class ContainedStructSummary : public StructSummary {
public:
  ReadSummary &getAccess() const;
  CollectionSummary &getContainer() const;
  llvm::CallInst &getCallInst() const;
  StructTypeSummary &getType() const override;

  std::string toString(std::string indent = "") const override;

protected:
  ReadSummary &access_to_container;

  ContainedStructSummary(ReadSummary &access_to_container);

  friend class AccessAnalysis;
};

/*
 * Referenced Struct Summary
 *
 * Represents an object or set of objects referenced by another MemOIR object.
 * This summary is flow-sensitive.
 */
class ReferencedStructSummary : public StructSummary {
public:
  ReadSummary &getAccess() const;
  llvm::CallInst &getCallInst() const;
  ReferenceTypeSummary &getReferenceType() const;

  StructTypeSummary &getType() const override;

protected:
  ReadSummary &access;

  ReferencedStructSummary(ReadSummary &access);

  friend class AccessAnalysis;
};

/*
 * Control PHI Struct Summary
 *
 * Represents a control PHI for incoming stucts along their control edges.
 */
class ControlPHIStructSummary : public StructSummary {
public:
  StructSummary &getIncomingStruct(unsigned idx) const;
  StructSummary &getIncomingStructForBlock(const llvm::BasicBlock &BB) const;
  llvm::BasicBlock &getIncomingBlock(unsigned idx) const;
  unsigned getNumIncoming() const;
  llvm::PHINode &getPHI() const;

  StructTypeSummary &getType() const override;

protected:
  llvm::PHINode &phi_node;
  map<llvm::BasicBlock *, StructSummary *> incoming;

  ControlPHIStructSummary(llvm::PHINode &phi_node,
                          map<llvm::BasicBlock *, StructSummary *> &incoming);

  friend class AccessAnalysis;
};

/*
 * Call PHI Struct Summary
 *
 * Represents a context-sensitive PHI for incoming stucts along their call edges
 * for a given argument.
 */
class CallPHIStructSummary : public StructSummary {
public:
  StructSummary &getIncomingStruct(uint64_t idx) const;
  StructSummary &getIncomingStructForCall(const llvm::CallBase &BB) const;
  llvm::CallBase &getIncomingCall(uint64_t idx) const;
  unsigned getNumIncoming() const;
  llvm::Argument &getArgument() const;

  StructTypeSummary &getType() const override;

protected:
  llvm::Argument &argument;
  vector<llvm::CallBase *> incoming_calls;
  map<llvm::CallBase *, StructSummary *> incoming;

  CallPHIStructSummary(llvm::PHINode &phi_node,
                       vector<llvm::CallBase *> &incoming_calls,
                       map<llvm::CallBase *, StructSummary *> &incoming);

  friend class AccessAnalysis;
};

/*
 * Access Info
 * Describes the nature of the access.
 * NOTE: Currently an instruction is only able to either read OR write.
 */
enum AccessMask {
  READ_MASK = 0,
  WRITE_MASK = 1,
  ASSOC_MASK = 2,
  INDEXED_MASK = 4,
  SLICE_MASK = 8,
};

enum AccessCode {
  READ = AccessMask::READ_MASK,
  WRITE = AccessMask::WRITE_MASK,
  ASSOC_READ = AccessMask::ASSOC_MASK | AccessMask::READ_MASK,
  ASSOC_WRITE = AccessMask::ASSOC_MASK | AccessMask::WRITE_MASK,
  INDEXED_READ = AccessMask::INDEXED_MASK | AccessMask::READ_MASK,
  INDEXED_WRITE = AccessMask::INDEXED_MASK | AccessMask::WRITE_MASK,
  SLICE_READ = AccessMask::SLICE_MASK | AccessMask::READ_MASK,
  SLICE_WRITE = AccessMask::SLICE_MASK | AccessMask::WRITE_MASK,
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
  CollectionSummary &getCollection() const;
  llvm::CallInst &getCallInst() const;
  TypeSummary &getType() const;

  AccessCode getAccessCode() const;
  bool isRead() const;
  bool isWrite() const;
  bool isAssociative() const;
  bool isIndexed() const;
  bool isSlice() const;

  friend bool operator<(const AccessSummary &l, const AccessSummary &r);
  friend std::ostream &operator<<(std::ostream &os, const AccessSummary &as);
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                       const AccessSummary &as);
  virtual std::string toString(std::string indent = "") const = 0;

private:
  AccessCode access_code;
  llvm::CallInst &call_inst;
  CollectionSummary &collection_accessed;

protected:
  AccessSummary(AccessCode access_code,
                llvm::CallInst &call_inst,
                CollectionSummary &collection_accessed);

  friend class AccessAnalysis;
};

/*
 * Read Summary
 *
 * Represents a read access to a MemOIR collection.
 */
class ReadSummary : public AccessSummary {
public:
  llvm::Value &getValueRead() const;

protected:
  llvm::Value &value_read;

  ReadSummary(AccessMask mask,
              llvm::CallInst &call_inst,
              CollectionSummary &collection_accessed,
              llvm::Value &value_read);

  friend class AccessAnalysis;
};

/*
 * Write Summary
 *
 * Represents a write access to MemOIR field.
 */
class WriteSummary : public AccessSummary {
public:
  llvm::Value &getValueWritten() const;

protected:
  llvm::Value &value_written;

  WriteSummary(AccessMask mask,
               llvm::CallInst &call_inst,
               CollectionSummary &collection_accessed,
               llvm::Value &value_written);

  friend class AccessAnalysis;
};

/*
 * Indexed Read Summary
 *
 * Represents a read access to an indexed memoir collection.
 */
class IndexedReadSummary : public ReadSummary {
public:
  uint64_t getNumDimensions() const;
  llvm::Value &getIndexValue(uint64_t dim_idx) const;

  std::string toString(std::string indent = "") const override;

protected:
  vector<llvm::Value *> indices;

  IndexedReadSummary(llvm::CallInst &call_inst,
                     CollectionSummary &collection_accessed,
                     llvm::Value &value_read,
                     vector<llvm::Value *> &indices);

  friend class AccessAnalysis;
};

/*
 * Indexed Write Summary
 *
 * Represents a write access to an indexed memoir collection.
 */
class IndexedWriteSummary : public WriteSummary {
public:
  uint64_t getNumDimensions() const;
  llvm::Value &getIndexValue(uint64_t dim_idx) const;

  std::string toString(std::string indent = "") const override;

protected:
  vector<llvm::Value *> indices;

  IndexedWriteSummary(llvm::CallInst &call_inst,
                      CollectionSummary &collection_accessed,
                      llvm::Value &value_written,
                      vector<llvm::Value *> &indices);

  friend class AccessAnalysis;
};

/*
 * Associative Read Summary
 *
 * Represents a read access to an associative memoir collection.
 */
class AssocReadSummary : public ReadSummary {
public:
  StructSummary &getKey() const;
  TypeSummary &getKeyType() const;

  std::string toString(std::string indent = "") const override;

protected:
  StructSummary &key;

  AssocReadSummary(llvm::CallInst &call_inst,
                   CollectionSummary &collection_accessed,
                   llvm::Value &value_read,
                   StructSummary &key);

  friend class AccessAnalysis;
};

/*
 * Associative Write Summary
 *
 * Represents a write access to an associative memoir collection.
 */
class AssocWriteSummary : public ReadSummary {
public:
  StructSummary &getKey() const;
  TypeSummary &getKeyType() const;

  std::string toString(std::string indent = "") const override;

protected:
  StructSummary &key;

  AssocWriteSummary(llvm::CallInst &call_inst,
                    CollectionSummary &collection_accessed,
                    llvm::Value &value_written,
                    StructSummary &key);

  friend class AccessAnalysis;
};

/*
 * Slice Read Summary
 *
 * Represents a read access to a sliced memoir collection.
 */
class SliceReadSummary : public ReadSummary {
public:
  llvm::Value &getLeft() const;
  llvm::Value &getRight() const;

  std::string toString(std::string indent = "") const override;

protected:
  llvm::Value &left;
  llvm::Value &right;

  SliceReadSummary(llvm::CallInst &call_inst,
                   CollectionSummary &collection_accessed,
                   llvm::Value &value_written,
                   llvm::Value &left,
                   llvm::Value &right);

  friend class AccessAnalysis;
};

/*
 * Slice Write Summary
 *
 * Represents a read access to a sliced memoir collection.
 */
class SliceWriteSummary : public WriteSummary {
public:
  llvm::Value &getLeft() const;
  llvm::Value &getRight() const;

  std::string toString(std::string indent = "") const override;

protected:
  llvm::Value &left;
  llvm::Value &right;

  SliceWriteSummary(llvm::CallInst &call_inst,
                    CollectionSummary &collection_accessed,
                    llvm::Value &value_written,
                    llvm::Value &left,
                    llvm::Value &right);

  friend class AccessAnalysis;
};

} // namespace llvm::memoir

#endif
