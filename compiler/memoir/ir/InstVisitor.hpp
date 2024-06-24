#ifndef COMMON_INST_VISITOR_H
#define COMMON_INST_VISITOR_H
#pragma once

#include "llvm/IR/InstVisitor.h"

#include "memoir/ir/Instructions.hpp"
#include "memoir/ir/MutOperations.hpp"

#include "memoir/support/Print.hpp"

#include "memoir/utility/FunctionNames.hpp"

namespace llvm::memoir {

#define DELEGATE_INST(CLASS_TO_VISIT)                                          \
  return static_cast<SubClass *>(this)->visit##CLASS_TO_VISIT(                 \
      static_cast<CLASS_TO_VISIT &>(I))

#define DELEGATE_MEMOIR(CLASS_TO_VISIT)                                        \
  return static_cast<SubClass *>(this)->visit##CLASS_TO_VISIT(                 \
      static_cast<CLASS_TO_VISIT &>(*(MemOIRInst::get(I))))

#define DELEGATE_LLVM(CLASS_TO_VISIT)                                          \
  return static_cast<SubClass *>(this)->visit##CLASS_TO_VISIT(                 \
      static_cast<CLASS_TO_VISIT &>(I.getCallInst()))

template <typename SubClass, typename RetTy = void>
class InstVisitor : public llvm::InstVisitor<SubClass, RetTy> {
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
#define HANDLE_INST(ENUM, FUNC, CLASS)                                         \
  case MemOIR_Func::ENUM:                                                      \
    DELEGATE_MEMOIR(CLASS);
#include "memoir/ir/MutOperations.def"
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
    DELEGATE_INST(MemOIRInst);                                                 \
  };

  // Access instruction hierarchy.
  RetTy visitAccessInst(AccessInst &I) {
    DELEGATE_INST(MemOIRInst);
  };
#define HANDLE_ACCESS_INST(ENUM, FUNC, CLASS) /* No handling. */

  // Read instruction hierarchy.
  RetTy visitReadInst(ReadInst &I) {
    DELEGATE_INST(AccessInst);
  };
  RetTy visitStructReadInst(StructReadInst &I) {
    DELEGATE_INST(ReadInst);
  };
  RetTy visitIndexReadInst(IndexReadInst &I) {
    DELEGATE_INST(ReadInst);
  };
  RetTy visitAssocReadInst(AssocReadInst &I) {
    DELEGATE_INST(ReadInst);
  };

  // Write instruction hierarchy.
  RetTy visitWriteInst(WriteInst &I) {
    DELEGATE_INST(AccessInst);
  };
  RetTy visitStructWriteInst(StructWriteInst &I) {
    DELEGATE_INST(WriteInst);
  };
  RetTy visitIndexWriteInst(IndexWriteInst &I) {
    DELEGATE_INST(WriteInst);
  };
  RetTy visitAssocWriteInst(AssocWriteInst &I) {
    DELEGATE_INST(WriteInst);
  };

  // Get instruction hierarchy.
  RetTy visitGetInst(GetInst &I) {
    DELEGATE_INST(AccessInst);
  };
  RetTy visitStructGetInst(StructGetInst &I) {
    DELEGATE_INST(GetInst);
  };
  RetTy visitIndexGetInst(IndexGetInst &I) {
    DELEGATE_INST(GetInst);
  };
  RetTy visitAssocGetInst(AssocGetInst &I) {
    DELEGATE_INST(GetInst);
  };
  RetTy visitAssocHasInst(AssocHasInst &I) {
    DELEGATE_INST(AccessInst);
  };

  // Type instruction hierarchy.
  RetTy visitTypeInst(TypeInst &I) {
    DELEGATE_INST(MemOIRInst);
  };
#define HANDLE_TYPE_INST(ENUM, FUNC, CLASS)                                    \
  RetTy visit##CLASS(CLASS &I) {                                               \
    DELEGATE_INST(TypeInst);                                                   \
  };

  // Alloc instruction hierarchy.
  RetTy visitAllocInst(AllocInst &I) {
    DELEGATE_INST(MemOIRInst);
  };
#define HANDLE_ALLOC_INST(ENUM, FUNC, CLASS)                                   \
  RetTy visit##CLASS(CLASS &I) {                                               \
    DELEGATE_INST(AllocInst);                                                  \
  };

  // Insert instruction hierarchy.
  RetTy visitInsertInst(InsertInst &I) {
    DELEGATE_INST(MemOIRInst);
  };
#define HANDLE_INSERT_INST(ENUM, FUNC, CLASS)                                  \
  RetTy visit##CLASS(CLASS &I) {                                               \
    DELEGATE_INST(InsertInst);                                                 \
  };
  RetTy visitSeqInsertInst(SeqInsertInst &I) {
    DELEGATE_INST(InsertInst);
  };
#define HANDLE_SEQ_INSERT_INST(ENUM, FUNC, CLASS) /* No handling. */

  // Remove instruction hierarchy.
  RetTy visitRemoveInst(RemoveInst &I) {
    DELEGATE_INST(MemOIRInst);
  };
#define HANDLE_REMOVE_INST(ENUM, FUNC, CLASS)                                  \
  RetTy visit##CLASS(CLASS &I) {                                               \
    DELEGATE_INST(RemoveInst);                                                 \
  };

  // Copy instruction hierarchy.
  RetTy visitCopyInst(CopyInst &I) {
    DELEGATE_INST(MemOIRInst);
  };
#define HANDLE_COPY_INST(ENUM, FUNC, CLASS)                                    \
  RetTy visit##CLASS(CLASS &I) {                                               \
    DELEGATE_INST(CopyInst);                                                   \
  };

  // Swap instruction hierarchy.
  RetTy visitSwapInst(SwapInst &I) {
    DELEGATE_INST(MemOIRInst);
  };
#define HANDLE_SWAP_INST(ENUM, FUNC, CLASS)                                    \
  RetTy visit##CLASS(CLASS &I) {                                               \
    DELEGATE_INST(SwapInst);                                                   \
  };
#include "memoir/ir/Instructions.def"

  // Mut instruction hierarchy.
  RetTy visitMutInst(MutInst &I) {
    DELEGATE_INST(MemOIRInst);
  };
#define HANDLE_INST(ENUM, FUNC, CLASS)                                         \
  RetTy visit##CLASS(CLASS &I) {                                               \
    DELEGATE_INST(MutInst);                                                    \
  };

  // Mut Write instruction hierarchy.
  RetTy visitMutWriteInst(MutWriteInst &I) {
    DELEGATE_INST(MutInst);
  };
  RetTy visitMutStructWriteInst(MutStructWriteInst &I) {
    DELEGATE_INST(MutWriteInst);
  };
  RetTy visitMutIndexWriteInst(MutIndexWriteInst &I) {
    DELEGATE_INST(MutWriteInst);
  };
  RetTy visitMutAssocWriteInst(MutAssocWriteInst &I) {
    DELEGATE_INST(MutWriteInst);
  };
#define HANDLE_WRITE_INST(ENUM, FUNC, CLASS) /* No handling. */

  // MutSeqInsertInst hierarchy.
  RetTy visitMutSeqInsertInst(MutSeqInsertInst &I) {
    DELEGATE_INST(MutInst);
  };
#define HANDLE_SEQ_INSERT_INST(ENUM, FUNC, CLASS) /* No handling. */

#include "memoir/ir/MutOperations.def"

protected:
};

} // namespace llvm::memoir

#endif
