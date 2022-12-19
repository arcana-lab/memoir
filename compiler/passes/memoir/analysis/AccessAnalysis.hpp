#ifndef COMMON_ACCESSANALYSIS_H
#define COMMON_ACCESSANALYSIS_H
#pragma once

#include <iostream>

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

#include "memoir/support/Assert.hpp"
#include "memoir/support/InternalDatatypes.hpp"

#include "memoir/utility/FunctionNames.hpp"
#include "memoir/utility/Metadata.hpp"

#include "memoir/analysis/AllocationAnalysis.hpp"
#include "memoir/analysis/CollectionAnalysis.hpp"
#include "memoir/analysis/TypeAnalysis.hpp"

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
class BaseStructSummary;
class ContainedStructSummary;
class NestedStructSummary;
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
   * @return The access summary, or NULL if use is not a memoir access.
   */
  AccessSummary *getAccessSummary(llvm::Instruction &inst);

  /**
   * Get the struct summary for a given llvm Value.
   *
   * @param value A reference to an llvm Use.
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
  TypeAnalysis &type_analysis;
  AllocationAnalysis &allocation_analysis;

  /*
   * Owned state
   */
  map<AllocationSummary *, BaseStructSummary *> base_struct_summaries;
  map<FieldArraySummary *, StructSummary *> nested_struct_summaries;
  map<AccessSummary *, ReferencedStructSummary *> reference_summaries;
  map<llvm::Value *, AccessSummary *> access_summaries;

  /*
   * Borrowed state
   */
  map<llvm::Value *, StructSummary *> value_to_struct_summaries;
  // map<FieldArraySummary *, set<AccessSummary *>> field_accesses;

  /*
   * Analysis driver functions
   */
  void initialize();
  void analyze();
  void analyzeStructs();
  void analyzeAccesses();

  /*
   * Internal helper functions for access summaries
   */
  AccessSummary *getAccessSummaryForCall(llvm::CallInst &call_inst);
  IndexedReadSummary *getIndexedReadSummaryForCall(
      llvm::CallInst &call_inst,
      CollectionSummary &collection_accessed,
      llvm::Value &value_read);
  IndexedWriteSummary *getIndexedWriteSummaryForCall(
      llvm::CallInst &call_inst,
      CollectionSummary &collection_accessed,
      llvm::Value &value_written);
  AssocReadSummary *getAssocReadSummaryForCall(
      llvm::CallInst &call_inst,
      CollectionSummary &collection_accessed,
      llvm::Value &value_read);
  AssocWriteSummary *getAssocWriteSummaryForCall(
      llvm::CallInst &call_inst,
      CollectionSummary &collection_accessed,
      llvm::Value &value_written);

  /*
   * Internal helper functions for struct summaries
   */
  StructSummary &getReadStructSummaries(llvm::CallInst &call_inst);
  ReferencedStructSummary &getReadReferenceSummaries(llvm::CallInst &call_inst);

  void invalidate();

  /*
   * Constructor
   */
  AccessAnalysis(llvm::Module &M);
  AccessAnalysis(llvm::Module &M,
                 TypeAnalysis &type_analysis,
                 AllocationAnalysis &allocation_analysis);

  /*
   * Factory
   */
  static map<llvm::Module *, AccessAnalysis *> analyses;
};

class ReadSummary;
class WriteSummary;

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
  llvm::Value &getKey() const;
  TypeSummary &getKeyType() const;
  StructSummary *getKeyAsStruct() const;
  CollectionSummary *getKeyAsCollection() const;

  std::string toString(std::string indent = "") const override;

protected:
  llvm::Value &key_as_value;
  StructSummary *key_as_struct;
  CollectionSummary *key_as_collection;

  AssocReadSummary(llvm::CallInst &call_inst,
                   CollectionSummary &collection_accessed,
                   llvm::Value &value_read,
                   llvm::Value &key_as_value);
  AssocReadSummary(llvm::CallInst &call_inst,
                   CollectionSummary &collection_accessed,
                   llvm::Value &value_read,
                   llvm::Value &key_as_value,
                   StructSummary &key_as_struct);
  AssocReadSummary(llvm::CallInst &call_inst,
                   CollectionSummary &collection_accessed,
                   llvm::Value &value_read,
                   llvm::Value &key_as_value,
                   CollectionSummary &key_as_collection);

  friend class AccessAnalysis;
};

/*
 * Associative Write Summary
 *
 * Represents a write access to an associative memoir collection.
 */
class AssocWriteSummary : public ReadSummary {
public:
  llvm::Value &getKey() const;
  TypeSummary &getKeyType() const;
  StructSummary *getKeyAsStruct() const;
  CollectionSummary *getKeyAsCollection() const;

  std::string toString(std::string indent = "") const override;

protected:
  llvm::Value &key_as_value;
  StructSummary *key_as_struct;
  CollectionSummary *key_as_collection;

  AssocWriteSummary(llvm::CallInst &call_inst,
                    CollectionSummary &collection_accessed,
                    llvm::Value &value_written,
                    llvm::Value &key_as_value);
  AssocWriteSummary(llvm::CallInst &call_inst,
                    CollectionSummary &collection_accessed,
                    llvm::Value &value_written,
                    llvm::Value &key_as_value,
                    StructSummary &key_as_struct);
  AssocWriteSummary(llvm::CallInst &call_inst,
                    CollectionSummary &collection_accessed,
                    llvm::Value &value_written,
                    llvm::Value &key_as_value,
                    CollectionSummary &key_as_collection);

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
