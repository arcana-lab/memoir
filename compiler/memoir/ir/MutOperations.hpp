#ifndef MEMOIR_MUTOPERATIONS_H
#define MEMOIR_MUTOPERATIONS_H
#pragma once

#include <cstddef>

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"

#include "memoir/support/Casting.hpp"

#include "memoir/utility/FunctionNames.hpp"

#include "memoir/ir/Types.hpp"

/*
 * Mut Operations as a wrapper of an LLVM Instruction.
 *
 * Author(s): Tommy McMichen
 * Created: October 23, 2023
 */

namespace llvm::memoir {

struct CollectionType;

// Abstract Mut Operation.
struct MutInst : public MemOIRInst {
  static MutInst *get(llvm::Instruction &I);

  MutInst(llvm::CallInst &call_inst) : MemOIRInst(call_inst) {}

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() > MemOIR_Func::BEGIN_MUT)
           && (I->getKind() < MemOIR_Func::END_MUT);
  };
};

// Abstract Mut Write.
struct MutWriteInst : public MutInst {
  llvm::Value &getValueWritten() const;
  llvm::Use &getValueWrittenAsUse() const;

  llvm::Value &getObjectOperand() const;
  llvm::Use &getObjectOperandAsUse() const;

  unsigned getNumberOfSubIndices() const;
  vector<llvm::Value *> getSubIndices() const;

  unsigned getSubIndex(unsigned sub_idx) const;
  llvm::Value &getSubIndexOperand(unsigned sub_idx) const;
  llvm::Use &getSubIndexOperandAsUse(unsigned sub_idx) const;

  Type &getElementType() const;

  static bool classof(const MemOIRInst *I) {
    return
#define HANDLE_WRITE_INST(ENUM, FUNC, CLASS)                                   \
  (I->getKind() == MemOIR_Func::ENUM) ||
#include "memoir/ir/MutOperations.def"
        false;
  };

protected:
  MutWriteInst(llvm::CallInst &call_inst) : MutInst(call_inst) {}

  friend struct MemOIRInst;
};

// Mutable write operations.
struct MutStructWriteInst : public MutWriteInst {
public:
  unsigned getFieldIndex() const;
  llvm::Value &getFieldIndexOperand() const;
  llvm::Use &getFieldIndexOperandAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return
#define HANDLE_STRUCT_WRITE_INST(ENUM, FUNC, CLASS)                            \
  (I->getKind() == MemOIR_Func::ENUM) ||
#include "memoir/ir/MutOperations.def"
        false;
  };

  std::string toString(std::string indent = "") const override;

protected:
  MutStructWriteInst(llvm::CallInst &call_inst) : MutWriteInst(call_inst) {}

  friend struct MemOIRInst;
};

struct MutIndexWriteInst : public MutWriteInst {
public:
  llvm::Value &getIndex() const;
  llvm::Use &getIndexAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return
#define HANDLE_INDEX_WRITE_INST(ENUM, FUNC, CLASS)                             \
  (I->getKind() == MemOIR_Func::ENUM) ||
#include "memoir/ir/MutOperations.def"
        false;
  };

  std::string toString(std::string indent = "") const override;

protected:
  MutIndexWriteInst(llvm::CallInst &call_inst) : MutWriteInst(call_inst) {}

  friend struct MemOIRInst;
};

struct MutAssocWriteInst : public MutWriteInst {
public:
  llvm::Value &getKeyOperand() const;
  llvm::Use &getKeyOperandAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return
#define HANDLE_ASSOC_WRITE_INST(ENUM, FUNC, CLASS)                             \
  (I->getKind() == MemOIR_Func::ENUM) ||
#include "memoir/ir/MutOperations.def"
        false;
  };

  std::string toString(std::string indent = "") const override;

protected:
  MutAssocWriteInst(llvm::CallInst &call_inst) : MutWriteInst(call_inst) {}

  friend struct MemOIRInst;
};

// Insert operations.
struct MutSeqInsertInst : public MutInst {
public:
  llvm::Value &getCollection() const;
  llvm::Use &getCollectionAsUse() const;

  llvm::Value &getInsertionPoint() const;
  llvm::Use &getInsertionPointAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::MUT_SEQ_INSERT);
  };

  std::string toString(std::string indent = "") const override;

protected:
  MutSeqInsertInst(llvm::CallInst &call_inst) : MutInst(call_inst) {}

  friend struct MemOIRInst;
};

struct MutSeqInsertValueInst : public MutInst {
public:
  llvm::Value &getCollection() const;
  llvm::Use &getCollectionAsUse() const;

  llvm::Value &getValueInserted() const;
  llvm::Use &getValueInsertedAsUse() const;

  llvm::Value &getInsertionPoint() const;
  llvm::Use &getInsertionPointAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return
#define HANDLE_SEQ_INSERT_INST(ENUM, FUNC, CLASS)                              \
  (I->getKind() == MemOIR_Func::ENUM) ||
#include "memoir/ir/MutOperations.def"
        false;
  };

  std::string toString(std::string indent = "") const override;

protected:
  MutSeqInsertValueInst(llvm::CallInst &call_inst) : MutInst(call_inst) {}

  friend struct MemOIRInst;
};

struct MutSeqInsertSeqInst : public MutInst {
public:
  llvm::Value &getCollection() const;
  llvm::Use &getCollectionAsUse() const;

  llvm::Value &getInsertedCollection() const;
  llvm::Use &getInsertedCollectionAsUse() const;

  llvm::Value &getInsertionPoint() const;
  llvm::Use &getInsertionPointAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::MUT_SEQ_INSERT_SEQ);
  };

  std::string toString(std::string indent = "") const override;

protected:
  MutSeqInsertSeqInst(llvm::CallInst &call_inst) : MutInst(call_inst) {}

  friend struct MemOIRInst;
};

// Remove operation
struct MutSeqRemoveInst : public MutInst {
public:
  llvm::Value &getCollection() const;
  llvm::Use &getCollectionAsUse() const;

  llvm::Value &getBeginIndex() const;
  llvm::Use &getBeginIndexAsUse() const;

  llvm::Value &getEndIndex() const;
  llvm::Use &getEndIndexAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::MUT_SEQ_REMOVE);
  };

  std::string toString(std::string indent = "") const override;

protected:
  MutSeqRemoveInst(llvm::CallInst &call_inst) : MutInst(call_inst) {}

  friend struct MemOIRInst;
};

// Append operation
struct MutSeqAppendInst : public MutInst {
public:
  llvm::Value &getCollection() const;
  llvm::Use &getCollectionAsUse() const;

  llvm::Value &getAppendedCollection() const;
  llvm::Use &getAppendedCollectionAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::MUT_SEQ_APPEND);
  };

  std::string toString(std::string indent = "") const override;

protected:
  MutSeqAppendInst(llvm::CallInst &call_inst) : MutInst(call_inst) {}

  friend struct MemOIRInst;
};

// Split operation
struct MutSeqSplitInst : public MutInst {
public:
  llvm::Value &getSplit() const;

  llvm::Value &getCollection() const;
  llvm::Use &getCollectionAsUse() const;

  llvm::Value &getBeginIndex() const;
  llvm::Use &getBeginIndexAsUse() const;
  llvm::Value &getEndIndex() const;
  llvm::Use &getEndIndexAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::MUT_SEQ_SPLIT);
  };

  std::string toString(std::string indent = "") const override;

protected:
  MutSeqSplitInst(llvm::CallInst &call_inst) : MutInst(call_inst) {}

  friend struct MemOIRInst;
};

// Swap operations
struct MutSeqSwapInst : public MutInst {
public:
  llvm::Value &getFromCollection() const;
  llvm::Use &getFromCollectionAsUse() const;

  llvm::Value &getBeginIndex() const;
  llvm::Use &getBeginIndexAsUse() const;

  llvm::Value &getEndIndex() const;
  llvm::Use &getEndIndexAsUse() const;

  llvm::Value &getToCollection() const;
  llvm::Use &getToCollectionAsUse() const;

  llvm::Value &getToBeginIndex() const;
  llvm::Use &getToBeginIndexAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::MUT_SEQ_SWAP);
  };

  std::string toString(std::string indent = "") const override;

protected:
  MutSeqSwapInst(llvm::CallInst &call_inst) : MutInst(call_inst) {}

  friend struct MemOIRInst;
};

struct MutSeqSwapWithinInst : public MutInst {
public:
  llvm::Value &getFromCollection() const;
  llvm::Use &getFromCollectionAsUse() const;

  llvm::Value &getBeginIndex() const;
  llvm::Use &getBeginIndexAsUse() const;

  llvm::Value &getEndIndex() const;
  llvm::Use &getEndIndexAsUse() const;

  llvm::Value &getToCollection() const;
  llvm::Use &getToCollectionAsUse() const;

  llvm::Value &getToBeginIndex() const;
  llvm::Use &getToBeginIndexAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::MUT_SEQ_SWAP_WITHIN);
  };

  std::string toString(std::string indent = "") const override;

protected:
  MutSeqSwapWithinInst(llvm::CallInst &call_inst) : MutInst(call_inst) {}

  friend struct MemOIRInst;
};

// Assoc operations.
struct MutAssocInsertInst : public MutInst {
public:
  llvm::Value &getCollection() const;
  llvm::Use &getCollectionAsUse() const;

  llvm::Value &getKeyOperand() const;
  llvm::Use &getKeyOperandAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::MUT_ASSOC_INSERT);
  };

  std::string toString(std::string indent = "") const override;

protected:
  MutAssocInsertInst(llvm::CallInst &call_inst) : MutInst(call_inst) {}

  friend struct MemOIRInst;
};

struct MutAssocRemoveInst : public MutInst {
public:
  llvm::Value &getCollection() const;
  llvm::Use &getCollectionAsUse() const;

  llvm::Value &getKeyOperand() const;
  llvm::Use &getKeyOperandAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::MUT_ASSOC_REMOVE);
  };

  std::string toString(std::string indent = "") const override;

protected:
  MutAssocRemoveInst(llvm::CallInst &call_inst) : MutInst(call_inst) {}

  friend struct MemOIRInst;
};

struct MutClearInst : public MutInst {
public:
  llvm::Value &getCollection() const;
  llvm::Use &getCollectionAsUse() const;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::MUT_CLEAR);
  };

  std::string toString(std::string indent = "") const override;

protected:
  MutClearInst(llvm::CallInst &call_inst) : MutInst(call_inst) {}

  friend struct MemOIRInst;
};

} // namespace llvm::memoir

#endif
