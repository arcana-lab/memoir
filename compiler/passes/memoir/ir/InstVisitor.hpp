#ifndef COMMON_INST_VISITOR_H
#define COMMON_INST_VISITOR_H
#pragma once

#include "llvm/IR/InstVisitor.h"

#include "memoir/ir/Instructions.hpp"
#include "memoir/ir/MutOperations.hpp"

#include "memoir/support/Print.hpp"

#include "memoir/utility/FunctionNames.hpp"

namespace llvm::memoir {

#define DELEGATE(CLASS_TO_VISIT)                                               \
  return static_cast<SubClass *>(this)->visit##CLASS_TO_VISIT(                 \
      static_cast<CLASS_TO_VISIT &>(I))

#define DELEGATE_MEMOIR(CLASS_TO_VISIT)                                        \
  return static_cast<SubClass *>(this)->visit##CLASS_TO_VISIT(                 \
      static_cast<CLASS_TO_VISIT &>(*(MemOIRInst::get(I))))

#define DELEGATE_LLVM(CLASS_TO_VISIT)                                          \
  return static_cast<SubClass *>(this)->visit##CLASS_TO_VISIT(                 \
      static_cast<CLASS_TO_VISIT &>(I.getCallInst()))

template <typename SubClass, typename RetTy = void>
struct InstVisitor : public llvm::InstVisitor<SubClass, RetTy> {
  friend class llvm::InstVisitor<SubClass, RetTy>;

public:
  RetTy visitInstruction(llvm::Instruction &I) {
    return static_cast<SubClass *>(this)->visitInstruction(I);
  }

  RetTy visitCallInst(llvm::CallInst &I) {
    static_assert(std::is_base_of<InstVisitor, SubClass>::value,
                  "Must pass the derived type to this template!");

    auto memoir_enum = FunctionNames::get_memoir_enum(I);

    switch (memoir_enum) {
      default:
        MEMOIR_UNREACHABLE("Unknown MemOIR instruction encountered!");
      case MemOIR_Func::NONE:
        return static_cast<SubClass *>(this)->visitLLVMCallInst(I);
#define HANDLE_INST(ENUM, FUNC, CLASS)                                         \
  case MemOIR_Func::ENUM:                                                      \
    DELEGATE_MEMOIR(CLASS);
#include "memoir/ir/Instructions.def"
    }
  };

  RetTy visitLLVMCallInst(llvm::CallInst &I) {
    return static_cast<SubClass *>(this)->visitInstruction(
        static_cast<llvm::Instruction &>(I));
  };
  // Root of instruction hierarchy
  RetTy visitMemOIRInst(MemOIRInst &I) {
    DELEGATE_LLVM(Instruction);
  };
#define HANDLE_INST(ENUM, FUNC, CLASS)                                         \
  RetTy visit##CLASS(CLASS &I) {                                               \
    DELEGATE(MemOIRInst);                                                      \
  };

  // Access instruction hierarchy.
  RetTy visitAccessInst(AccessInst &I) {
    DELEGATE(MemOIRInst);
  };
#define HANDLE_ACCESS_INST(ENUM, FUNC, CLASS) /* No handling. */

  // Read instruction hierarchy.
  RetTy visitReadInst(ReadInst &I) {
    DELEGATE(AccessInst);
  };
  RetTy visitStructReadInst(StructReadInst &I) {
    DELEGATE(ReadInst);
  };
  RetTy visitIndexReadInst(IndexReadInst &I) {
    DELEGATE(ReadInst);
  };
  RetTy visitAssocReadInst(AssocReadInst &I) {
    DELEGATE(ReadInst);
  };

  // Write instruction hierarchy.
  RetTy visitWriteInst(WriteInst &I) {
    DELEGATE(AccessInst);
  };
  RetTy visitStructWriteInst(StructWriteInst &I) {
    DELEGATE(WriteInst);
  };
  RetTy visitIndexWriteInst(IndexWriteInst &I) {
    DELEGATE(WriteInst);
  };
  RetTy visitAssocWriteInst(AssocWriteInst &I) {
    DELEGATE(WriteInst);
  };

  // Get instruction hierarchy.
  RetTy visitGetInst(GetInst &I) {
    DELEGATE(AccessInst);
  };
  RetTy visitStructGetInst(StructGetInst &I) {
    DELEGATE(GetInst);
  };
  RetTy visitIndexGetInst(IndexGetInst &I) {
    DELEGATE(GetInst);
  };
  RetTy visitAssocGetInst(AssocGetInst &I) {
    DELEGATE(GetInst);
  };
  RetTy visitAssocHasInst(AssocHasInst &I) {
    DELEGATE(AccessInst);
  };

  // Type instruction hierarchy.
  RetTy visitTypeInst(TypeInst &I) {
    DELEGATE(MemOIRInst);
  };
#define HANDLE_TYPE_INST(ENUM, FUNC, CLASS)                                    \
  RetTy visit##CLASS(CLASS &I) {                                               \
    DELEGATE(TypeInst);                                                        \
  };

  // Alloc instruction hierarchy.
  RetTy visitAllocInst(AllocInst &I) {
    DELEGATE(MemOIRInst);
  };
#define HANDLE_ALLOC_INST(ENUM, FUNC, CLASS)                                   \
  RetTy visit##CLASS(CLASS &I) {                                               \
    DELEGATE(AllocInst);                                                       \
  };

  // Insert instruction hierarchy.
  RetTy visitInsertInst(InsertInst &I) {
    DELEGATE(MemOIRInst);
  };
#define HANDLE_INSERT_INST(ENUM, FUNC, CLASS)                                  \
  RetTy visit##CLASS(CLASS &I) {                                               \
    DELEGATE(InsertInst);                                                      \
  };
  RetTy visitSeqInsertInst(SeqInsertInst &I) {
    DELEGATE(InsertInst);
  };
#define HANDLE_SEQ_INSERT_INST(ENUM, FUNC, CLASS) /* No handling. */

  // Remove instruction hierarchy.
  RetTy visitRemoveInst(RemoveInst &I) {
    DELEGATE(MemOIRInst);
  };
#define HANDLE_REMOVE_INST(ENUM, FUNC, CLASS)                                  \
  RetTy visit##CLASS(CLASS &I) {                                               \
    DELEGATE(RemoveInst);                                                      \
  };

  // Copy instruction hierarchy.
  RetTy visitCopyInst(CopyInst &I) {
    DELEGATE(MemOIRInst);
  };
#define HANDLE_COPY_INST(ENUM, FUNC, CLASS)                                    \
  RetTy visit##CLASS(CLASS &I) {                                               \
    DELEGATE(CopyInst);                                                        \
  };

  // Swap instruction hierarchy.
  RetTy visitSwapInst(SwapInst &I) {
    DELEGATE(MemOIRInst);
  };
#define HANDLE_SWAP_INST(ENUM, FUNC, CLASS)                                    \
  RetTy visit##CLASS(CLASS &I) {                                               \
    DELEGATE(SwapInst);                                                        \
  };

#include "memoir/ir/Instructions.def"

  // Mut instruction hierarchy.
  RetTy visitMutInst(MutInst &I) {
    DELEGATE(MemOIRInst);
  };
#define HANDLE_INST(ENUM, FUNC, CLASS)                                         \
  RetTy visit##CLASS(CLASS &I) {                                               \
    DELEGATE(MutInst);                                                         \
  };

  // Mut Write instruction hierarchy.
  RetTy visitMutWriteInst(MutWriteInst &I) {
    DELEGATE(MutInst);
  };
  RetTy visitMutStructWriteInst(MutStructWriteInst &I) {
    DELEGATE(MutWriteInst);
  };
  RetTy visitMutIndexWriteInst(MutIndexWriteInst &I) {
    DELEGATE(MutWriteInst);
  };
  RetTy visitMutAssocWriteInst(MutAssocWriteInst &I) {
    DELEGATE(MutWriteInst);
  };
#define HANDLE_WRITE_INST(ENUM, FUNC, CLASS) /* No handling. */

  // MutSeqInsertInst hierarchy.
  RetTy visitMutSeqInsertInst(MutSeqInsertInst &I) {
    DELEGATE(MutInst);
  };
#define HANDLE_SEQ_INSERT_INST(ENUM, FUNC, CLASS) /* No handling. */

#include "memoir/ir/MutOperations.def"

protected:
};

} // namespace llvm::memoir

#endif
