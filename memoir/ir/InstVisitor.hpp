#ifndef COMMON_INST_VISITOR_H
#define COMMON_INST_VISITOR_H
#pragma once

#include "llvm/IR/InstVisitor.h"

#include "memoir/ir/Instructions.hpp"
#include "memoir/ir/MutOperations.hpp"

#include "memoir/support/Print.hpp"

#include "memoir/utility/FunctionNames.hpp"

namespace memoir {

#define DELEGATE_INST(CLASS_TO_VISIT)                                          \
  return static_cast<SubClass *>(this)->visit##CLASS_TO_VISIT(                 \
      static_cast<CLASS_TO_VISIT &>(I))

#define DELEGATE_MEMOIR(CLASS_TO_VISIT)                                        \
  return static_cast<SubClass *>(this)->visit##CLASS_TO_VISIT(                 \
      static_cast<CLASS_TO_VISIT &>(*(MemOIRInst::get(I))))

#define DELEGATE_LLVM(CLASS_TO_VISIT)                                          \
  return static_cast<SubClass *>(this)->visit##CLASS_TO_VISIT(                 \
      static_cast<llvm::CLASS_TO_VISIT &>(I.getCallInst()))

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
#define HANDLE_ACCESS_INST(ENUM, FUNC, CLASS)                                  \
  RetTy visit##CLASS(CLASS &I) {                                               \
    DELEGATE_INST(AccessInst);                                                 \
  };

  RetTy visitReadInst(ReadInst &I) {
    DELEGATE_INST(AccessInst);
  };
#define HANDLE_READ_INST(ENUM, FUNC, CLASS) /* Do nothing. */

  // UpdateInst hierarchy
  RetTy visitUpdateInst(UpdateInst &I) {
    DELEGATE_INST(AccessInst);
  }
#define HANDLE_UPDATE_INST(ENUM, FUNC, CLASS)                                  \
  RetTy visit##CLASS(CLASS &I) {                                               \
    DELEGATE_INST(UpdateInst);                                                 \
  }

  RetTy visitWriteInst(WriteInst &I) {
    DELEGATE_INST(UpdateInst);
  };
#define HANDLE_WRITE_INST(ENUM, FUNC, CLASS) /* Do nothing */

  // Type instruction hierarchy.
  RetTy visitTypeInst(TypeInst &I) {
    DELEGATE_INST(MemOIRInst);
  };
#define HANDLE_TYPE_INST(ENUM, FUNC, CLASS)                                    \
  RetTy visit##CLASS(CLASS &I) {                                               \
    DELEGATE_INST(TypeInst);                                                   \
  };

  // Fold instruction hierarchy.
  RetTy visitFoldInst(FoldInst &I) {
    DELEGATE_INST(AccessInst);
  };
#define HANDLE_FOLD_INST(ENUM, FUNC, CLASS) /* No handling */
#include "memoir/ir/Instructions.def"

  // Mut instruction hierarchy.
  RetTy visitMutInst(MutInst &I) {
    DELEGATE_INST(AccessInst);
  };
#define HANDLE_INST(ENUM, FUNC, CLASS)                                         \
  RetTy visit##CLASS(CLASS &I) {                                               \
    DELEGATE_INST(MutInst);                                                    \
  };

  // Mut Write instruction hierarchy.
  RetTy visitMutWriteInst(MutWriteInst &I) {
    DELEGATE_INST(MutInst);
  };
#define HANDLE_WRITE_INST(ENUM, FUNC, CLASS) /* No handling. */

#include "memoir/ir/MutOperations.def"

protected:
};

} // namespace memoir

#endif
