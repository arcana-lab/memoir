#ifndef COMMON_STRUCTS_H
#define COMMON_STRUCTS_H
#pragma once

#include "memoir/support/Assert.hpp"
#include "memoir/support/InternalDatatypes.hpp"

#include "memoir/ir/Collections.hpp"
#include "memoir/ir/Instructions.hpp"
#include "memoir/ir/Types.hpp"

/*
 * This file provides a substrate for MemOIR structs.
 *
 * Author(s): Tommy McMichen
 * Created: December 19, 2022
 */

namespace llvm::memoir {

enum class StructCode {
  BASE,
  NESTED,
  CONTAINED,
  REFERENCED,
  CONTROL_PHI,
  CALL_PHI
};

struct StructAllocInst;
struct ReadInst;
struct GetInst;

/*
 * Struct Summary
 *
 * Represents a dynamic instance of a MemOIR struct.
 */
class Struct {
public:
  StructCode getCode() const;

  virtual StructType &getType() const = 0;

  friend std::ostream &operator<<(std::ostream &os, const Struct &summary);
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                       const Struct &summary);
  virtual std::string toString(std::string indent = "") const = 0;

protected:
  StructCode code;

  Struct(StructCode code);

  friend class AccessAnalysis;
};

/*
 * Base Struct Summary
 *
 * Represents a base allocation of a MemOIR object.
 */
class BaseStruct : public Struct {
public:
  StructAllocInst &getAllocInst() const;
  StructType &getType() const override;

  std::string toString(std::string indent = "") const override;

protected:
  StructAllocInst &allocation;

  BaseStruct(StructAllocInst &allocation);

  friend class AccessAnalysis;
};

/*
 * Contained Struct Summary
 *
 * Represents a struct contained within a MemOIR collection.
 * This could be an element of a tensor of structs, etc.
 */
class ContainedStruct : public Struct {
public:
  GetInst &getAccess() const;
  virtual Collection &getContainer() const;

  StructType &getType() const override;

  std::string toString(std::string indent = "") const override;

protected:
  GetInst &access;

  ContainedStruct(GetInst &access_to_container);

  friend class AccessAnalysis;
};

/*
 * Nested Struct Summary
 *
 * Represents a nested struct within another MemOIR struct.
 * This could be a struct within a struct.
 */
class NestedStruct : public ContainedStruct {
public:
  FieldArray &getFieldArray() const;

  std::string toString(std::string indent = "") const override;

protected:
  NestedStruct(GetInst &access);

  friend class AccessAnalysis;
};

/*
 * Referenced Struct Summary
 *
 * Represents an object or set of objects referenced by another MemOIR object.
 * This summary is flow-sensitive.
 */
class ReferencedStruct : public Struct {
public:
  ReadInst &getAccess() const;
  ReferenceType &getReferenceType() const;

  StructType &getType() const override;

protected:
  ReadInst &access;

  ReferencedStruct(ReadInst &access);

  friend class AccessAnalysis;
};

/*
 * Control PHI Struct Summary
 *
 * Represents a control PHI for incoming stucts along their control edges.
 */
class ControlPHIStruct : public Struct {
public:
  Struct &getIncomingStruct(unsigned idx) const;
  Struct &getIncomingStructForBlock(const llvm::BasicBlock &BB) const;
  llvm::BasicBlock &getIncomingBlock(unsigned idx) const;
  unsigned getNumIncoming() const;
  llvm::PHINode &getPHI() const;

  StructType &getType() const override;

protected:
  llvm::PHINode &phi_node;
  map<llvm::BasicBlock *, Struct *> incoming;

  ControlPHIStruct(llvm::PHINode &phi_node,
                   map<llvm::BasicBlock *, Struct *> &incoming);

  friend class AccessAnalysis;
};

/*
 * Call PHI Struct Summary
 *
 * Represents a context-sensitive PHI for incoming stucts along their call edges
 * for a given argument.
 */
class CallPHIStruct : public Struct {
public:
  Struct &getIncomingStruct(uint64_t idx) const;
  Struct &getIncomingStructForCall(const llvm::CallBase &BB) const;
  llvm::CallBase &getIncomingCall(uint64_t idx) const;
  uint64_t getNumIncoming() const;
  llvm::Argument &getArgument() const;

  StructType &getType() const override;

protected:
  llvm::Argument &argument;
  vector<llvm::CallBase *> incoming_calls;
  map<llvm::CallBase *, Struct *> incoming;

  CallPHIStruct(llvm::Argument &argument,
                vector<llvm::CallBase *> &incoming_calls,
                map<llvm::CallBase *, Struct *> &incoming);

  friend class AccessAnalysis;
};

} // namespace llvm::memoir

#endif
