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

struct MutInst : public AccessInst {
  static MutInst *get(llvm::Instruction &I);

  MutInst(llvm::CallInst &call_inst) : AccessInst(call_inst) {}

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() > MemOIR_Func::BEGIN_MUT)
           && (I->getKind() < MemOIR_Func::END_MUT);
  };
};

struct MutWriteInst : public MutInst {
  llvm::Value &getValueWritten() const;
  llvm::Use &getValueWrittenAsUse() const;

  llvm::Value &getObject() const override;
  llvm::Use &getObjectAsUse() const override;

  static bool classof(const MemOIRInst *I) {
    return
#define HANDLE_WRITE_INST(ENUM, FUNC, CLASS)                                   \
  (I->getKind() == MemOIR_Func::ENUM) ||
#include "memoir/ir/MutOperations.def"
        false;
  };

  std::string toString() const override;

protected:
  MutWriteInst(llvm::CallInst &call_inst) : MutInst(call_inst) {}

  friend struct MemOIRInst;
};

struct MutInsertInst : public MutInst {
public:
  llvm::Value &getObject() const override;
  llvm::Use &getObjectAsUse() const override;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::MUT_INSERT);
  };

  std::string toString() const override;

protected:
  MutInsertInst(llvm::CallInst &call_inst) : MutInst(call_inst) {}

  friend struct MemOIRInst;
};

struct MutRemoveInst : public MutInst {
public:
  llvm::Value &getObject() const override;
  llvm::Use &getObjectAsUse() const override;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::MUT_REMOVE);
  };

  std::string toString() const override;

protected:
  MutRemoveInst(llvm::CallInst &call_inst) : MutInst(call_inst) {}

  friend struct MemOIRInst;
};

struct MutClearInst : public MutInst {
public:
  llvm::Value &getObject() const override;
  llvm::Use &getObjectAsUse() const override;

  static bool classof(const MemOIRInst *I) {
    return (I->getKind() == MemOIR_Func::MUT_CLEAR);
  };

  std::string toString() const override;

protected:
  MutClearInst(llvm::CallInst &call_inst) : MutInst(call_inst) {}

  friend struct MemOIRInst;
};

} // namespace llvm::memoir

#endif
